// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

#include "lapi.h"
#include "lgc.h"
#include "llimits.h"
#include "lobject.h"
#include "lstate.h"

#include "mlua/event.h"
#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/platform.h"
#include "mlua/util.h"

// TODO: Stop using uint64_t for deadlines; use only lower 32 bits of clock

static char const mlua_Thread_name[] = "mlua.Thread";

// Non-running thread stack indexes.
#define FP_NEXT (-1)
#define FP_FLAGS (-2)
#define FP_DEADLINE (-3)
#define FP_COUNT 3

#define FLAGS_STATE 0x3
#define STATE_ACTIVE 0
#define STATE_SUSPENDED 1
#define STATE_TIMER 2
#define STATE_DEAD 3

// Upvalue indexes for main.
#define UV_HEAD 1
#define UV_TAIL 2
#define UV_TIMERS 3
#define UV_THREADS 4
#define UV_JOINERS 5
#define UV_NAMES 6
#define MAIN_UV_COUNT 6

static uint8_t main_tag;

static inline lua_State* main_thread(lua_State* ls) {
    return G(ls)->mainthread;
}

// Push a thread value.
static inline void push_thread(lua_State* ls, lua_State* thread) {
    lua_lock(ls);
    setthvalue(ls, s2v(ls->top.p), thread);
    api_incr_top(ls);
    lua_unlock(ls);
}

// Push a thread value, or nil if the pointer is NULL.
static inline void push_thread_or_nil(lua_State* ls, lua_State* thread) {
    if (thread == NULL) {
        lua_pushnil(ls);
    } else {
        push_thread(ls, thread);
    }
}

// Push a value from the main() function to a potentially different stack.
static inline void push_main_value(lua_State* ls, int arg) {
    lua_State* main = G(ls)->mainthread;
    lua_pushvalue(main, arg);
    lua_xmove(main, ls, 1);
}

// Replace the NEXT value of a non-running thread.
static inline void replace_next(lua_State* thread, lua_State* next) {
    lua_pop(thread, 1);
    push_thread(thread, next);
}

// Replace the FLAGS value of a non-running thread.
static inline void replace_flags(lua_State* thread, lua_Integer flags) {
    lua_pushinteger(thread, flags);
    lua_replace(thread, FP_FLAGS - 1);
}

static inline void push_main(lua_State* ls) {
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &main_tag);
}

static void print_state(lua_State* ls, lua_State* running, char const* msg) {
    printf("# %s\n#   Running: %p\n#   Active:", msg, running);
    lua_State* main = main_thread(ls);
    lua_State* tail = lua_tothread(main, lua_upvalueindex(UV_TAIL));
    lua_State* ts = lua_tothread(main, lua_upvalueindex(UV_HEAD));
    int i = 0;
    while (ts != NULL) {
        printf(" %p%s", ts, ts == tail ? "*" : "");
        ts = lua_tothread(ts, FP_NEXT);
        if (++i > 10) break;
    }
    printf("\n#   Timers:");
    ts = lua_tothread(main, lua_upvalueindex(UV_TIMERS));
    i = 0;
    while (ts != NULL) {
        printf(" %p", ts);
        ts = lua_tothread(ts, FP_NEXT);
        if (++i > 10) break;
    }
    printf("\n");
}

static int thread_state(lua_State* thread) {
    switch (lua_status(thread)) {
    case LUA_OK: {
        lua_Debug ar;
        if (!lua_getstack(thread, 0, &ar) && lua_gettop(thread) == 0) {
            return STATE_DEAD;
        }
        __attribute__((fallthrough));
    }
    case LUA_YIELD:
        return lua_tointeger(thread, FP_FLAGS) & FLAGS_STATE;
    default:
        return STATE_DEAD;
    }
}

static void remove_timer(lua_State* ls, lua_State* thread) {
    lua_State* prev = lua_tothread(ls, lua_upvalueindex(UV_TIMERS));
    if (thread == prev) {  // thread == prev?
        lua_xmove(thread, ls, 1);
        lua_replace(ls, lua_upvalueindex(UV_TIMERS));  // TIMERS = thread.NEXT
        lua_pushnil(thread);  // thread.NEXT = nil
        return;
    }
    for (;;) {
        // next = prev.NEXT
        lua_State* next = lua_tothread(prev, FP_NEXT);
        if (next == NULL) return;
        if (next == thread) {  // next == thread?
            lua_pop(prev, 1);
            lua_xmove(thread, prev, 1);  // prev.NEXT = thread.NEXT
            lua_pushnil(thread);  // thread.NEXT = nil
            return;
        }
        prev = next;
    }
}

static void activate(lua_State* ls, lua_State* thread, int main) {
    // thread.FLAGS = STATE_ACTIVE
    replace_flags(thread, STATE_ACTIVE);
    // tail = TAIL
    lua_getupvalue(ls, main, UV_TAIL);
    lua_State* tail = lua_tothread(ls, -1);
    lua_pop(ls, 1);
    if (tail == NULL) {
        // main.HEAD = thread
        push_thread(ls, thread);
        lua_setupvalue(ls, main, UV_HEAD);
    } else {
        // tail.NEXT = thread
        lua_pop(tail, 1);
        push_thread(tail, thread);
    }
    // main.TAIL = thread
    push_thread(ls, thread);
    lua_setupvalue(ls, main, UV_TAIL);
}

static void main_activate(lua_State* ls, lua_State* thread) {
    // thread.FLAGS = STATE_ACTIVE
    replace_flags(thread, STATE_ACTIVE);
    // tail = TAIL
    lua_State* tail = lua_tothread(ls, lua_upvalueindex(UV_TAIL));
    if (tail == NULL) {
        // HEAD = thread
        push_thread(ls, thread);
        lua_replace(ls, lua_upvalueindex(UV_HEAD));
    } else {
        // tail.NEXT = thread
        lua_pop(tail, 1);
        push_thread(tail, thread);
    }
    // TAIL = thread
    push_thread(ls, thread);
    lua_replace(ls, lua_upvalueindex(UV_TAIL));
}

static bool resume(lua_State* ls, lua_State* thread) {
    int state = thread_state(thread);
    if (state == STATE_ACTIVE || state == STATE_DEAD) return false;
    if (state == STATE_TIMER) remove_timer(ls, thread);
    main_activate(ls, thread);
    return true;
}

static int Thread_name(lua_State* ls) {
    lua_State* self = mlua_check_thread(ls, 1);
    push_main_value(ls, lua_upvalueindex(UV_NAMES));
    lua_pushvalue(ls, 1);
    if (lua_rawget(ls, -2) != LUA_TNIL) return 1;
    return lua_pushfstring(ls, "%p", self), 1;
}

static int Thread_is_alive(lua_State* ls) {
    lua_State* self = mlua_check_thread(ls, 1);
    lua_pushboolean(ls, self == ls || thread_state(self) != STATE_DEAD);
    return 1;
}

static int Thread_is_waiting(lua_State* ls) {
    lua_State* self = mlua_check_thread(ls, 1);
    if (self == ls) return lua_pushboolean(ls, false), 1;
    int state = thread_state(self);
    lua_pushboolean(ls, state == STATE_SUSPENDED || state == STATE_TIMER);
    return 1;
}

static int Thread_resume(lua_State* ls) {
    lua_State* self = mlua_check_thread(ls, 1);
    if (self == ls) return lua_pushboolean(ls, false), 1;
    lua_State* main = main_thread(ls);
    return lua_pushboolean(ls, resume(main, self)), 1;
}

static int Thread_kill(lua_State* ls) {
    lua_State* self = mlua_check_thread(ls, 1);
    // TODO: Better error message
    if (self == ls) return luaL_error(ls, "cannot close a running coroutine");
    int state = thread_state(self);
    if (state == STATE_DEAD) return lua_pushboolean(ls, false), 1;

    // Remove the thread from the timer list if necessary.
    lua_State* main = main_thread(ls);
    // TODO: Remove from active queue if STATE_ACTIVE
    if (state == STATE_TIMER) remove_timer(main, self);

    // Close the Lua thread and store the termination in self.DEADLINE.
    lua_xmove(self, ls, 1);  // next = self.NEXT
    lua_pop(self, FP_COUNT - 1);
    if (lua_closethread(self, ls) == LUA_OK) lua_pushnil(self);
    lua_pushinteger(self, STATE_DEAD);  // self.FLAGS = STATE_DEAD
    lua_xmove(ls, self, 1);  // self.NEXT = next

    // Resume joiners.
    push_main_value(ls, lua_upvalueindex(UV_JOINERS));
    lua_pushvalue(ls, 1);
    int typ = lua_rawget(ls, -2);  // joiners = main.JOINERS[self]
    if (typ != LUA_TNIL) {
        // main.JOINERS[self] = nil
        lua_pushvalue(ls, 1);
        lua_pushnil(ls);
        lua_rawset(ls, -3);
        switch (typ) {
        case LUA_TTHREAD: {  // Resume one joiner
            resume(main, lua_tothread(ls, -1));
            lua_pop(ls, 1);  // Remove joiners
            break;
        }
        case LUA_TTABLE: {  // Resume multiple joiners
            lua_pushnil(ls);
            while (lua_next(ls, -2)) {
                lua_pop(ls, 1);  // Remove value
                resume(main, lua_tothread(ls, -1));
            }
            lua_pop(ls, 1);  // Remove joiners
            break;
        }}
    }

    // main.THREADS[self] = nil
    push_main_value(ls, lua_upvalueindex(UV_THREADS));
    lua_pushvalue(ls, 1);
    lua_pushnil(ls);
    lua_rawset(ls, -3);
    return lua_pushboolean(ls, true), 1;
}

static int Thread_join_1(lua_State* ls, int status, lua_KContext ctx);
static int Thread_join_2(lua_State* ls, lua_State* self);

static int Thread_join(lua_State* ls) {
    // TODO: Remove from joiners list on exit
    lua_State* self = mlua_check_thread(ls, 1);
    lua_settop(ls, 1);
    if (thread_state(self) == STATE_DEAD) return Thread_join_2(ls, self);

    // Add the running thread to main.JOINERS[self].
    push_main_value(ls, lua_upvalueindex(UV_JOINERS));
    lua_pushvalue(ls, 1);
    switch (lua_rawget(ls, -2)) {  // joiners = main.JOINERS[self]
    case LUA_TNIL:
        lua_pop(ls, 1);  // Remove joiners
        // main.JOINERS[self] = running
        lua_pushvalue(ls, 1);
        push_thread(ls, ls);
        lua_rawset(ls, -3);
        break;
    case LUA_TTHREAD:
        // main.JOINERS[self] = {[running] = true, [joiners] = true}
        lua_pushvalue(ls, 1);
        lua_createtable(ls, 0, 2);
        push_thread(ls, ls);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -3);
        lua_rotate(ls, -3, -1);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -3);
        lua_rawset(ls, -3);
        break;
    case LUA_TTABLE:
        // main.JOINERS[self][running] = true
        lua_pushvalue(ls, 1);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -3);
        lua_pop(ls, 1);  // Remove joiners
        break;
    }
    lua_settop(ls, 1);
    lua_pushnil(ls);
    return mlua_event_yield(ls, 1, &Thread_join_1, (lua_KContext)self);
}

static int Thread_join_1(lua_State* ls, int status, lua_KContext ctx) {
    lua_State* self = (lua_State*)ctx;
    if (thread_state(self) == STATE_DEAD) return Thread_join_2(ls, self);
    lua_pushnil(ls);
    return mlua_event_yield(ls, 1, &Thread_join_1, (lua_KContext)self);
}

static int Thread_join_2(lua_State* ls, lua_State* self) {
    if (lua_isnil(self, FP_DEADLINE)) return 0;
    lua_pushvalue(self, FP_DEADLINE);
    lua_xmove(self, ls, 1);
    return lua_error(ls);
}

static int mod_running(lua_State* ls) {
    return lua_pushthread(ls), 1;
}

static int mod_yield(lua_State* ls) {
    lua_settop(ls, 0);
    return lua_yield(ls, 0);
}

static int mod_suspend(lua_State* ls) {
    if (!lua_isnoneornil(ls, 1)) mlua_check_int64(ls, 1);
    lua_settop(ls, 1);
    return lua_yield(ls, 1);
}

static int mod_yield_enabled(lua_State* ls) {
    mlua_require(ls, "mlua.event", true);
    lua_getfield(ls, -1, "yield_enabled");
    lua_rotate(ls, 1, 1);
    lua_pop(ls, 1);
    lua_call(ls, lua_gettop(ls) - 1, 1);
    return 1;
}

static int mod_start(lua_State* ls) {
    luaL_checktype(ls, 1, LUA_TFUNCTION);
    bool has_name = !lua_isnoneornil(ls, 2);
    if (has_name) luaL_checktype(ls, 2, LUA_TSTRING);

    // Create the thread.
    lua_State* thread = lua_newthread(ls);
    lua_pushvalue(ls, 1);
    lua_xmove(ls, thread, 1);
    lua_pushnil(thread);                    // FP_DEADLINE
    lua_pushinteger(thread, STATE_ACTIVE);  // FP_FLAGS
    lua_pushnil(thread);                    // FP_NEXT

    // Set the name if provided.
    // TODO: Convert to direct access to main() upvalues. This requires a
    //       special case for when main() isn't running yet (ls == main).
    push_main(ls);
    if (has_name) {
        lua_getupvalue(ls, -1, UV_NAMES);
        push_thread(ls, thread);
        lua_pushvalue(ls, 2);
        lua_rawset(ls, -3);
        lua_pop(ls, 1);
    }

    // Add the thread to main.THREADS.
    lua_getupvalue(ls, -1, UV_THREADS);
    push_thread(ls, thread);
    lua_pushboolean(ls, true);
    lua_rawset(ls, -3);  // main.THREADS[thread] = true
    lua_pop(ls, 1);  // Remove THREADS

    // Add the thread to the active queue.
    // TODO: Add to queue directly, as FLAGS is already set
    activate(ls, thread, lua_absindex(ls, -1));
    lua_pop(ls, 1);  // Remove main
    return 1;
}

static int mod_shutdown(lua_State* ls) {
    lua_settop(ls, 1);
    lua_pushnil(ls);
    lua_rotate(ls, 1, 1);
    return lua_yield(ls, 2);
}

static int main_done(lua_State* ls) {
    lua_settop(ls, 0);

    // Get a reference to main.
    lua_Debug ar;
    lua_getstack(ls, 2, &ar);  // main is 2 levels above
    lua_getinfo(ls, "f", &ar);

    // Close all threads.
    lua_getupvalue(ls, 1, UV_THREADS);
    lua_pushnil(ls);
    while (lua_next(ls, -2)) {
        lua_pop(ls, 1);  // Remove value
        lua_State* thread = lua_tothread(ls, -1);
        lua_pop(thread, FP_COUNT);
        lua_closethread(thread, ls);
    }
    lua_pop(ls, 1);  // Remove THREADS

    // Clear main upvalues.
    // TODO: Create a new THREADS
    for (int i = 1; i <= MAIN_UV_COUNT; ++i) {
        lua_pushnil(ls);
        lua_setupvalue(ls, 1, i);
    }
    return 0;
}

static int mod_main(lua_State* ls) {
    // Defer main_done().
    lua_settop(ls, 0);
    lua_pushcfunction(ls, &main_done);
    lua_toclose(ls, -1);

    // Run the main scheduling loop.
    lua_State* running = NULL;
    for (;;) {
        // Dispatch events.
        uint64_t min_ticks, deadline;
        mlua_ticks_range(&min_ticks, &deadline);
        lua_State* timers = lua_tothread(ls, lua_upvalueindex(UV_TIMERS));
        if (running != NULL || !lua_isnil(ls, lua_upvalueindex(UV_TAIL))) {
            deadline = min_ticks;
        } else if (timers != NULL) {
            deadline = mlua_to_int64(timers, FP_DEADLINE);
        }
        mlua_event_dispatch(ls, deadline, &resume);

        // Resume threads whose deadline has elapsed.
        timers = lua_tothread(ls, lua_upvalueindex(UV_TIMERS));
        lua_State* tail = lua_tothread(ls, lua_upvalueindex(UV_TAIL));
        uint64_t ticks = mlua_ticks();
        if (timers != NULL
                && (uint64_t)mlua_to_int64(timers, FP_DEADLINE) <= ticks) {
            // Append the timer list to the tail of the active queue, then cut
            // the combined list at the first thread whose deadline hasn't
            // elapsed yet.
            if (tail == NULL) {
                // HEAD = timers
                push_thread(ls, timers);
                lua_replace(ls, lua_upvalueindex(UV_HEAD));
            } else {
                replace_next(tail, timers);  // tail.NEXT = timers
            }
            for (;;) {
                tail = timers;
                // tail.FLAGS = STATE_ACTIVE
                replace_flags(tail, STATE_ACTIVE);
                // timers = tail.NEXT
                timers = lua_tothread(tail, FP_NEXT);
                if (timers == NULL ||
                        (uint64_t)mlua_to_int64(timers, FP_DEADLINE) > ticks) {
                    // TIMERS = timers
                    push_thread_or_nil(ls, timers);
                    lua_replace(ls, lua_upvalueindex(UV_TIMERS));
                    break;
                }
            }
            // TAIL = tail
            push_thread(ls, tail);
            lua_replace(ls, lua_upvalueindex(UV_TAIL));
            // tail.NEXT = nil
            lua_pop(tail, 1);
            lua_pushnil(tail);
        }

        // If the previous running thread is still active, move it to the end of
        // the active queue, after threads resumed by events or timers. Then get
        // the thread at the head of the active queue, skipping dead ones.
        if (tail != NULL) {
            if (running != NULL) {
                replace_next(tail, running);  // tail.NEXT = running
                // TAIL = tail = running;
                tail = running;
                push_thread(ls, tail);
                lua_replace(ls, lua_upvalueindex(UV_TAIL));
            }
            // head = HEAD
            lua_State* head = lua_tothread(ls, lua_upvalueindex(UV_HEAD));
            for (;;) {
                running = head;
                head = lua_tothread(head, FP_NEXT);
                if (thread_state(running) != STATE_DEAD) break;
                // running.NEXT = nil
                lua_pop(running, 1);
                lua_pushnil(running);
                if (head == NULL) {
                    running = NULL;
                    break;
                }
            }
            if (head == NULL) {
                // HEAD = TAIL = nil
                lua_pushnil(ls);
                lua_replace(ls, lua_upvalueindex(UV_HEAD));
                lua_pushnil(ls);
                lua_replace(ls, lua_upvalueindex(UV_TAIL));
            } else {
                // HEAD = head
                push_thread(ls, head);
                lua_replace(ls, lua_upvalueindex(UV_HEAD));
            }
        }
        if (running == NULL) continue;

        // Resume the selected thread.
        lua_pop(running, FP_COUNT);
        int nres;
        if (lua_resume(running, ls, 0, &nres) != LUA_YIELD) {
            // Close the Lua thread and store the termination in DEADLINE.
            if (lua_closethread(running, ls) == LUA_OK) lua_pushnil(running);
            lua_pushinteger(running, STATE_DEAD);
            lua_pushnil(running);

            // Resume joiners.
            // joiners = JOINERS[running]
            push_thread(ls, running);
            int typ = lua_rawget(ls, lua_upvalueindex(UV_JOINERS));
            if (typ != LUA_TNIL) {
                // JOINERS[running] = nil
                push_thread(ls, running);
                lua_pushnil(ls);
                lua_rawset(ls, lua_upvalueindex(UV_JOINERS));
                switch (typ) {
                case LUA_TTHREAD: {  // Resume one joiner
                    resume(ls, lua_tothread(ls, -1));
                    break;
                }
                case LUA_TTABLE: {  // Resume multiple joiners
                    lua_pushnil(ls);
                    while (lua_next(ls, -2)) {
                        lua_pop(ls, 1);  // Remove value
                        resume(ls, lua_tothread(ls, -1));
                    }
                    break;
                }}
            }
            lua_pop(ls, 1);  // Remove joiners
            // THREADS[running] = nil
            push_thread(ls, running);
            lua_pushnil(ls);
            lua_rawset(ls, lua_upvalueindex(UV_THREADS));
            running = NULL;
            continue;
        }

        if (nres > 2) lua_pop(running, nres - 2);
        if (nres == 2) {  // Shutdown
            lua_xmove(running, ls, 1);
            return 1;
        }
        if (nres == 0) {
            // Keep running in the active queue.
            lua_pushnil(running);
            lua_pushinteger(running, STATE_ACTIVE);
            lua_pushnil(running);
            continue;
        }

        // Suspend running.
        if (lua_isnil(running, -1)) {
            // Suspend indefinitely.
            lua_pushinteger(running, STATE_SUSPENDED);
            lua_pushnil(running);
            running = NULL;
            continue;
        }

        // Add running to the timer list.
        deadline = mlua_to_int64(running, -1);
        lua_pushinteger(running, STATE_TIMER);
        timers = lua_tothread(ls, lua_upvalueindex(UV_TIMERS));
        if (timers == NULL ||
                deadline < (uint64_t)mlua_to_int64(timers, FP_DEADLINE)) {
            push_thread_or_nil(running, timers);  // running.NEXT = timers
            // TIMERS = running
            push_thread(ls, running);
            lua_replace(ls, lua_upvalueindex(UV_TIMERS));
            running = NULL;
            continue;
        }
        for (;;) {
            // next = timers.NEXT
            lua_State* next = lua_tothread(timers, FP_NEXT);
            if (next == NULL ||
                    deadline < (uint64_t)mlua_to_int64(next, FP_DEADLINE)) {
                push_thread_or_nil(running, next);  // running.NEXT = next
                replace_next(timers, running);  // timers.NEXT = running
                break;
            }
            timers = next;
        }
        running = NULL;
    }
}

MLUA_SYMBOLS(Thread_syms) = {
    MLUA_SYM_F(name, Thread_),
    MLUA_SYM_F(is_alive, Thread_),
    MLUA_SYM_F(is_waiting, Thread_),
};

#define Thread___close Thread_join

MLUA_SYMBOLS_NOHASH(Thread_syms_nh) = {
    MLUA_SYM_F_NH(start, mod_),
    MLUA_SYM_F_NH(shutdown, mod_),
    MLUA_SYM_F_NH(resume, Thread_),
    MLUA_SYM_F_NH(kill, Thread_),
    MLUA_SYM_F_NH(join, Thread_),
    MLUA_SYM_F_NH(__close, Thread_),
};

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(running, mod_),
    MLUA_SYM_F(yield, mod_),
    MLUA_SYM_F(suspend, mod_),
    MLUA_SYM_F(yield_enabled, mod_),
    MLUA_SYM_F(start, mod_),
    MLUA_SYM_F(shutdown, mod_),
};

MLUA_OPEN_MODULE(mlua.thread) {
    mlua_event_require(ls);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the Thread class.
    lua_pushthread(ls);
    mlua_new_class(ls, mlua_Thread_name, Thread_syms, true);
    mlua_set_fields(ls, Thread_syms_nh);
    lua_setmetatable(ls, -2);
    lua_pop(ls, 1);

    // Create the main() closure.
    lua_pushnil(ls);
    lua_pushnil(ls);
    lua_pushnil(ls);
    lua_createtable(ls, 0, 0);  // THREADS
    lua_createtable(ls, 0, 0);  // JOINERS
    luaL_setmetatable(ls, mlua_WeakK_name);
    lua_createtable(ls, 0, 0);  // NAMES
    luaL_setmetatable(ls, mlua_WeakK_name);
    lua_pushcclosure(ls, &mod_main, MAIN_UV_COUNT);
    lua_pushvalue(ls, -1);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, &main_tag);
    lua_setfield(ls, -2, "main");
    return 1;
}
