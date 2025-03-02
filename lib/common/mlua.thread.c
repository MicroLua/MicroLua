// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/thread.h"

#include <assert.h>
#include <inttypes.h>

#include "lapi.h"
#include "lgc.h"
#include "llimits.h"
#include "lobject.h"
#include "lstate.h"
#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/platform.h"

void mlua_thread_require(lua_State* ls) {
    mlua_require(ls, "mlua.thread", false);
}

static char const mlua_Thread_name[] = "mlua.Thread";

// Data stored in the per-thread extra space returned by lua_getextraspace().
typedef struct ThreadExtra {
    uint64_t deadline;
    uint8_t state;
    uint8_t flags;
} ThreadExtra;

static_assert(sizeof(ThreadExtra) <= LUA_EXTRASPACE,
              "LUA_EXTRASPACE too small");

// Thread states, as stored in ThreadExtra.state.
typedef enum ThreadState {
    STATE_ACTIVE,
    STATE_SUSPENDED,
    STATE_TIMER,
    STATE_DEAD,
} ThreadState;

// Thread flags, as stored in ThreadExtra.flags.
typedef enum ThreadFlags {
    FLAGS_BLOCKING = 1u << 0,
} ThreadFlags;

// Non-running thread stack indexes.
#define FP_NEXT (-1)
#define FP_COUNT 1

// Upvalue indexes for main.
typedef enum MainUpvalueIndex {
    UV_HEAD = 1,
    UV_TAIL,
    UV_TIMERS,
    UV_THREADS,
    UV_JOINERS,
    UV_NAMES,
} MainUpvalueIndex;

// Return a reference to the main thread.
static inline lua_State* main_thread(lua_State* ls) {
#ifdef mainthread  // Defined in lstate.h in Lua >=5.5
    return mainthread(G(ls));
#else
    return G(ls)->mainthread;
#endif
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
    lua_State* main = main_thread(ls);
    lua_pushvalue(main, arg);
    lua_xmove(main, ls, 1);
}

// Return the ThreadExtra structure for a thread.
static inline ThreadExtra* thread_extra(lua_State* thread) {
    return (ThreadExtra*)lua_getextraspace(thread);
}

// Replace the NEXT value of a non-running thread.
static inline void replace_next(lua_State* thread, lua_State* next) {
    lua_pop(thread, 1);
    push_thread(thread, next);
}

static void print_main_state(lua_State* ls, lua_State* running,
                             char const* msg) {
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
        return thread_extra(thread)->state;
    default:
        return STATE_DEAD;
    }
}

int mlua_thread_suspend(lua_State* ls, lua_KFunction cont, lua_KContext ctx,
                        int index) {
    if (index == 0) {
        lua_pushnil(ls);
    } else {
        lua_pushvalue(ls, index);
    }
    return mlua_thread_yield(ls, 1, cont, ctx);
}

lua_State* mlua_check_thread(lua_State* ls, int arg) {
    lua_State* thread = lua_tothread(ls, arg);
    luaL_argexpected(ls, thread != NULL, arg, "thread");
    return thread;
}

int mlua_thread_meta(lua_State* ls, char const* name) {
    lua_pushthread(ls);
    int res = luaL_getmetafield(ls, -1, name);
    lua_remove(ls, res != LUA_TNIL ? -2 : -1);
    return res;
}

static void remove_timer(lua_State* main, lua_State* thread) {
    // prev = TIMERS
    lua_State* prev = lua_tothread(main, lua_upvalueindex(UV_TIMERS));
    if (thread == prev) {
        lua_xmove(thread, main, 1);
        lua_replace(main, lua_upvalueindex(UV_TIMERS));  // TIMERS = thread.NEXT
        lua_pushnil(thread);  // thread.NEXT = nil
        return;
    }
    for (;;) {
        lua_State* next = lua_tothread(prev, FP_NEXT);  // next = prev.NEXT
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

static void activate(lua_State* main, lua_State* thread) {
    // tail = TAIL
    lua_State* tail = lua_tothread(main, lua_upvalueindex(UV_TAIL));
    if (tail == NULL) {
        // HEAD = thread
        push_thread(main, thread);
        lua_replace(main, lua_upvalueindex(UV_HEAD));
    } else {
        // tail.NEXT = thread
        lua_pop(tail, 1);
        push_thread(tail, thread);
    }
    // TAIL = thread
    push_thread(main, thread);
    lua_replace(main, lua_upvalueindex(UV_TAIL));
}

static bool resume(lua_State* main, lua_State* thread) {
    int state = thread_state(thread);
    if (state == STATE_ACTIVE || state == STATE_DEAD) return false;
    if (state == STATE_TIMER) remove_timer(main, thread);
    thread_extra(thread)->state = STATE_ACTIVE;
    activate(main, thread);
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
    if (self == ls) return luaL_error(ls, "thread cannot kill itself");
    int state = thread_state(self);
    if (state == STATE_DEAD) return lua_pushboolean(ls, false), 1;

    // Remove the thread from the timer list if necessary.
    lua_State* main = main_thread(ls);
    if (state == STATE_TIMER) remove_timer(main, self);

    // Close the Lua thread and store the termination below self.NEXT.
    lua_xmove(self, ls, 1);  // next = self.NEXT
    if (lua_closethread(self, ls) == LUA_OK) lua_pushnil(self);
    thread_extra(self)->state = STATE_DEAD;
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

void mlua_thread_kill(lua_State* ls) {
    lua_pushcfunction(ls, &Thread_kill);
    lua_rotate(ls, -2, 1);
    lua_call(ls, 1, 1);
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
    return mlua_thread_yield(ls, 1, &Thread_join_1, (lua_KContext)self);
}

static int Thread_join_1(lua_State* ls, int status, lua_KContext ctx) {
    lua_State* self = (lua_State*)ctx;
    if (thread_state(self) == STATE_DEAD) return Thread_join_2(ls, self);
    lua_pushnil(ls);
    return mlua_thread_yield(ls, 1, &Thread_join_1, (lua_KContext)self);
}

static int Thread_join_2(lua_State* ls, lua_State* self) {
    // TODO: Return results of thread function to caller
    if (lua_isnil(self, FP_NEXT - 1)) return 0;
    lua_pushvalue(self, FP_NEXT - 1);
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
    luaL_argexpected(ls, lua_isnoneornil(ls, 1) || mlua_is_time(ls, 1),
                     1, "integer or Int64");
    lua_settop(ls, 1);
    return lua_yield(ls, 1);
}

bool mlua_thread_blocking(lua_State* ls) {
    return (thread_extra(ls)->flags & FLAGS_BLOCKING) != 0;
}

static int mod_blocking(lua_State* ls) {
    bool b = mlua_thread_blocking(ls);
    if (!lua_isnoneornil(ls, 1)) {
        if (lua_toboolean(ls, 1)) {
            thread_extra(ls)->flags |= FLAGS_BLOCKING;
        } else {
            thread_extra(ls)->flags &= ~FLAGS_BLOCKING;
        }
    }
    return lua_pushboolean(ls, b), 1;
}

static int mod_start(lua_State* ls) {
    luaL_checktype(ls, 1, LUA_TFUNCTION);
    bool has_name = !lua_isnoneornil(ls, 2);
    if (has_name) luaL_checktype(ls, 2, LUA_TSTRING);

    // Create the thread.
    lua_State* thread = lua_newthread(ls);
    ThreadExtra* ext = thread_extra(thread);
    ext->state = STATE_ACTIVE;
    ext->flags = thread_extra(ls)->flags;
    lua_pushvalue(ls, 1);
    lua_xmove(ls, thread, 1);
    lua_pushnil(thread);  // thread.NEXT = nil

    lua_State* main = main_thread(ls);
    if (luai_likely(ls != main)) {
        // Set the name if provided.
        if (has_name) {
            push_main_value(ls, lua_upvalueindex(UV_NAMES));
            push_thread(ls, thread);
            lua_pushvalue(ls, 2);
            lua_rawset(ls, -3);  // main.NAMES[thread] = name
            lua_pop(ls, 1);  // Remove NAMES
        }

        // Add the thread to main.THREADS.
        push_main_value(ls, lua_upvalueindex(UV_THREADS));
        push_thread(ls, thread);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -3);  // main.THREADS[thread] = true
        lua_pop(ls, 1);  // Remove THREADS

        // Add the thread to the active queue.
        activate(main, thread);
        return 1;
    }

    // main() hasn't been called yet. Get a reference to it through the module.
    mlua_require(ls, "mlua.thread", true);
    lua_getfield(ls, -1, "main");
    lua_remove(ls, -2);
    ext->flags = 0;  // Don't inherit flags from main thread

    // Set the name if provided.
    if (has_name) {
        lua_getupvalue(ls, -1, UV_NAMES);
        push_thread(ls, thread);
        lua_pushvalue(ls, 2);
        lua_rawset(ls, -3);  // main.NAMES[thread] = name
        lua_pop(ls, 1);  // Remove NAMES
    }

    // Add the thread to main.THREADS.
    lua_getupvalue(ls, -1, UV_THREADS);
    push_thread(ls, thread);
    lua_pushboolean(ls, true);
    lua_rawset(ls, -3);  // main.THREADS[thread] = true
    lua_pop(ls, 1);  // Remove THREADS

    // Add the thread to the active queue.
    // tail = main.TAIL
    lua_getupvalue(ls, -1, UV_TAIL);
    lua_State* tail = lua_tothread(ls, -1);
    lua_pop(ls, 1);
    if (tail == NULL) {
        // main.HEAD = thread
        push_thread(ls, thread);
        lua_setupvalue(ls, -2, UV_HEAD);
    } else {
        // tail.NEXT = thread
        lua_pop(tail, 1);
        push_thread(tail, thread);
    }
    // main.TAIL = thread
    push_thread(ls, thread);
    lua_setupvalue(ls, -2, UV_TAIL);
    lua_pop(ls, 1);  // Remove main
    return 1;
}

void mlua_thread_start(lua_State* ls) {
    lua_pushcfunction(ls, &mod_start);
    lua_rotate(ls, -2, 1);
    lua_call(ls, 1, 1);
}

static int mod_shutdown(lua_State* ls) {
    lua_settop(ls, 2);
    return lua_yield(ls, 2);
}

static int mod_stats(lua_State* ls) {
#if MLUA_THREAD_STATS
    MLuaGlobal* g = mlua_global(ls);
    lua_pushinteger(ls, g->thread_dispatches);
    lua_pushinteger(ls, g->thread_waits);
    lua_pushinteger(ls, g->thread_resumes);
    return 3;
#else
    return 0;
#endif
}

static void reset_main_state(lua_State* ls, int arg) {
    for (int i = UV_HEAD; i <= UV_TIMERS; ++i) {
        lua_pushnil(ls);
        lua_setupvalue(ls, arg, i);
    }
    lua_createtable(ls, 0, 0);
    lua_setupvalue(ls, arg, UV_THREADS);
    lua_createtable(ls, 0, 0);
    luaL_setmetatable(ls, mlua_WeakK_name);
    lua_setupvalue(ls, arg, UV_JOINERS);
    lua_createtable(ls, 0, 0);
    luaL_setmetatable(ls, mlua_WeakK_name);
    lua_setupvalue(ls, arg, UV_NAMES);
}

static int main_done(lua_State* ls) {
    lua_settop(ls, 0);

    // Close all threads.
    lua_getupvalue(ls, lua_upvalueindex(1), UV_THREADS);
    lua_pushnil(ls);
    while (lua_next(ls, -2)) {
        lua_pop(ls, 1);  // Remove value
        lua_State* thread = lua_tothread(ls, -1);
        lua_pop(thread, FP_COUNT);
        lua_closethread(thread, ls);
    }
    lua_pop(ls, 1);  // Remove THREADS

    reset_main_state(ls, lua_upvalueindex(1));
    return 0;
}

static int mod_main(lua_State* ls) {
    lua_settop(ls, 0);

    // Defer main_done().
    lua_Debug ar;
    lua_getstack(ls, 0, &ar);
    lua_getinfo(ls, "f", &ar);  // Get a reference to main()
    lua_pushcclosure(ls, &main_done, 1);
    lua_toclose(ls, -1);

    // Run the main scheduling loop.
    lua_State* running = NULL;
    for (;;) {
        // Dispatch events.
        uint64_t deadline = MLUA_TICKS_MAX;
        lua_State* timers = lua_tothread(ls, lua_upvalueindex(UV_TIMERS));
        if (running != NULL || !lua_isnil(ls, lua_upvalueindex(UV_TAIL))) {
            deadline = MLUA_TICKS_MIN;
        } else if (timers != NULL) {
            deadline = thread_extra(timers)->deadline;
        }
        mlua_event_dispatch(ls, deadline);

        // Resume threads whose deadline has elapsed.
        timers = lua_tothread(ls, lua_upvalueindex(UV_TIMERS));
        lua_State* tail = lua_tothread(ls, lua_upvalueindex(UV_TAIL));
        uint64_t ticks = mlua_ticks64();
        if (timers != NULL && thread_extra(timers)->deadline <= ticks) {
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
                thread_extra(tail)->state = STATE_ACTIVE;
                // timers = tail.NEXT
                timers = lua_tothread(tail, FP_NEXT);
                if (timers == NULL || thread_extra(timers)->deadline > ticks) {
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
#if MLUA_THREAD_STATS
        ++mlua_global(ls)->thread_resumes;
#endif
        lua_pop(running, FP_COUNT);
        int nres;
        if (lua_resume(running, ls, 0, &nres) != LUA_YIELD) {
            // Close the Lua thread and store the termination below NEXT.
            if (lua_closethread(running, ls) == LUA_OK) lua_pushnil(running);
            thread_extra(running)->state = STATE_DEAD;
            lua_pushnil(running);  // running.NEXT = nil

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

        if (nres > 2) {
            lua_pop(running, nres - 2);
            nres = 2;
        }
        if (nres == 2) {  // Shutdown
            bool raise = lua_toboolean(running, -1);
            lua_pop(running, 1);
            lua_xmove(running, ls, 1);
            lua_pushnil(running);  // running.NEXT = nil
            if (raise) return lua_error(ls);
            return 1;
        }
        if (nres == 0) {
            // Keep running in the active queue.
            lua_pushnil(running);  // running.NEXT = nil
            continue;
        }

        // Suspend running.
        if (lua_isnil(running, -1)) {
            // Suspend indefinitely.
            lua_pop(running, 1);  // Remove deadline
            thread_extra(running)->state = STATE_SUSPENDED;
            lua_pushnil(running);  // running.NEXT = nil
            running = NULL;
            continue;
        }

        // Add running to the timer list.
        deadline = mlua_to_time(running, -1);
        lua_pop(running, 1);  // Remove deadline
        ThreadExtra* extra = thread_extra(running);
        extra->deadline = deadline;
        extra->state = STATE_TIMER;
        timers = lua_tothread(ls, lua_upvalueindex(UV_TIMERS));
        if (timers == NULL || deadline < thread_extra(timers)->deadline) {
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
            if (next == NULL || deadline < thread_extra(next)->deadline) {
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
    MLUA_SYM_F(blocking, mod_),
    MLUA_SYM_F(start, mod_),
    MLUA_SYM_F(shutdown, mod_),
    MLUA_SYM_F(stats, mod_),
};

MLUA_OPEN_MODULE(mlua.thread) {
    mlua_require(ls, "mlua.int64", false);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the Thread class.
    lua_pushthread(ls);
    mlua_new_class(ls, mlua_Thread_name, Thread_syms, Thread_syms_nh);
    lua_setmetatable(ls, -2);
    lua_pop(ls, 1);

    // Create the main() closure.
    for (int i = UV_HEAD; i <= UV_NAMES; ++i) lua_pushnil(ls);
    lua_pushcclosure(ls, &mod_main, UV_NAMES - UV_HEAD + 1);
    reset_main_state(ls, lua_absindex(ls, -1));
    lua_setfield(ls, -2, "main");
    return 1;
}

static void watch_event(lua_State* ls, MLuaEvent const* ev) {
    lua_pushthread(ls);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, ev);
}

static void watch_event_from_thread(lua_State* ls, MLuaEvent const* ev,
                                    int thread) {
    lua_pushvalue(ls, thread);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, ev);
}

static void unwatch_event(lua_State* ls, MLuaEvent const* ev) {
    if (!mlua_event_enabled(ev)) return;
    mlua_event_remove_watcher(ls, ev);
}

static inline void next_event(MLuaEvent const** evs, unsigned int* mask) {
    unsigned int bit = MLUA_CTZ(*mask) + 1;
    *evs += bit;
    *mask >>= bit;
}

static void watch_events(lua_State* ls, MLuaEvent const* evs,
                         unsigned int mask) {
    for (;;) {
        watch_event(ls, evs);
        if (mask == 0) return;
        next_event(&evs, &mask);
    }
}

static void unwatch_events(lua_State* ls, MLuaEvent const* evs,
                           unsigned int mask) {
    for (;;) {
        unwatch_event(ls, evs);
        if (mask == 0) return;
        next_event(&evs, &mask);
    }
}

bool mlua_event_resume_watcher(lua_State* ls, MLuaEvent const* ev) {
    bool res = false;
    if (lua_rawgetp(ls, LUA_REGISTRYINDEX, ev) != LUA_TNIL) {
        res = resume(ls, lua_tothread(ls, -1));
    }
    lua_pop(ls, 1);
    return res;
}

void mlua_event_remove_watcher(lua_State* ls, MLuaEvent const* ev) {
    lua_pushnil(ls);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, ev);
}

bool mlua_event_can_wait(lua_State* ls, MLuaEvent const* evs,
                         unsigned int mask) {
    if (mlua_thread_blocking(ls)) return false;
    for (;;) {
        if (!mlua_event_enabled(evs)) return false;
        if (mask == 0) return true;
        next_event(&evs, &mask);
    }
}

static int mlua_event_wait_1(lua_State* ls, MLuaEvent const* evs,
                             unsigned int mask, MLuaEventLoopFn loop,
                             int index);
static int mlua_event_wait_2(lua_State* ls, int status, lua_KContext ctx);

int mlua_event_wait(lua_State* ls, MLuaEvent const* evs,
                          unsigned int mask, MLuaEventLoopFn loop, int index) {
    int res = loop(ls, false);
    if (res >= 0) return res;
    if (index != 0) {
        if (lua_isnoneornil(ls, index)) {
            index = 0;
        } else {
            luaL_argexpected(ls, mlua_is_time(ls, index), index,
                             "integer or Int64");
        }
    }
    // TODO: Use a deferred to unwatch
    watch_events(ls, evs, mask);
    return mlua_event_wait_1(ls, evs, mask, loop, index);
}

static int mlua_event_wait_1(lua_State* ls, MLuaEvent const* evs,
                             unsigned int mask, MLuaEventLoopFn loop,
                             int index) {
    lua_pushinteger(ls, mask);
    lua_pushlightuserdata(ls, loop);
    lua_pushinteger(ls, index);
    return mlua_thread_suspend(ls, &mlua_event_wait_2, (lua_KContext)evs,
                               index);
}

static int mlua_event_wait_2(lua_State* ls, int status,
                                   lua_KContext ctx) {
    MLuaEvent* evs = (MLuaEvent*)ctx;
    unsigned int mask = lua_tointeger(ls, -3);
    MLuaEventLoopFn loop = (MLuaEventLoopFn)lua_touserdata(ls, -2);
    int index = lua_tointeger(ls, -1);
    lua_pop(ls, 3);  // Restore the stack for loop
    int res = loop(ls, index != 0 && mlua_time_reached(ls, index));
    if (res < 0) return mlua_event_wait_1(ls, evs, mask, loop, index);
    unwatch_events(ls, evs, mask);
    return res;
}

static int handler_thread_1(lua_State* ls, int status, lua_KContext ctx);
static int handler_thread_2(lua_State* ls, int status, lua_KContext ctx);
static int handler_thread_done(lua_State* ls);

static int handler_thread(lua_State* ls) {
    // Set up a deferred to clean up on exit.
    lua_pushvalue(ls, lua_upvalueindex(2));  // done
    lua_pushvalue(ls, lua_upvalueindex(3));  // event
    lua_pushcclosure(ls, &handler_thread_done, 2);
    lua_toclose(ls, -1);

    // Start the event handling loop.
    return handler_thread_1(ls, LUA_OK, 0);
}

static int handler_thread_1(lua_State* ls, int status, lua_KContext ctx) {
    // Call the event handler.
    lua_pushvalue(ls, lua_upvalueindex(1));  // handler
    return mlua_callk(ls, 0, 1, handler_thread_2, 0);
}

static int handler_thread_2(lua_State* ls, int status, lua_KContext ctx) {
    // Suspend until the event gets pending or the deadline returned by the
    // handler elapses.
    return mlua_thread_yield(ls, 1, &handler_thread_1, 0);
}

static int handler_thread_done(lua_State* ls) {
    // Stop watching the event.
    MLuaEvent* ev = lua_touserdata(ls, lua_upvalueindex(2));
    unwatch_event(ls, ev);

    // Call the "handler done" callback.
    if (lua_isnil(ls, lua_upvalueindex(1))) return 0;
    lua_pushvalue(ls, lua_upvalueindex(1));
    return mlua_callk(ls, 0, 0, mlua_cont_return, 0);
}

int mlua_event_handle(lua_State* ls, MLuaEvent* ev, lua_KFunction cont,
                      lua_KContext ctx) {
    lua_pushlightuserdata(ls, ev);
    lua_pushcclosure(ls, &handler_thread, 3);
    mlua_thread_start(ls);
    watch_event_from_thread(ls, ev, -1);
    // If the handler thread is killed before it gets a chance to run, it will
    // remain as a watcher and therefore leak. Since we yield here, this can
    // only happen from other threads that are on the active queue right now,
    // and only during this scheduling round. This is an unlikely sequence of
    // events, so we don't bother handling it.
    return mlua_thread_yield(ls, 0, cont, ctx);
}

void mlua_event_stop_handler(lua_State* ls, MLuaEvent const* ev) {
    if (lua_rawgetp(ls, LUA_REGISTRYINDEX, ev) != LUA_TNIL) {
        mlua_thread_kill(ls);
        lua_pop(ls, 1);
    } else {
        lua_pop(ls, 1);
    }
}

int mlua_event_push_handler_thread(lua_State* ls, MLuaEvent const* ev) {
    return lua_rawgetp(ls, LUA_REGISTRYINDEX, ev);
}
