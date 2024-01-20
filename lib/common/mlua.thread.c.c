// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/platform.h"
#include "mlua/util.h"

static char const mlua_Thread_name[] = "mlua.Thread";

// Uservalue indexes for Thread.
#define UV_THREAD 1
#define UV_NEXT 2
#define UV_JOINERS 3
#define UV_MAIN 4
#define THREAD_UV_COUNT 4

static uint8_t main_tag;

// Upvalue indexes for main.
#define UV_TAIL 1
#define UV_TIMERS 2
#define UV_THREADS 3
#define MAIN_UV_COUNT 3

// Special values for Thread.deadline.
#define DL_DEAD ((uint64_t)-1)
#define DL_ACTIVE ((uint64_t)-2)
#define DL_INFINITE ((uint64_t)-3)

typedef struct Thread {
    uint64_t deadline;
} Thread;

static inline Thread* to_Thread(lua_State* ls, int arg) {
    return (Thread*)lua_touserdata(ls, arg);
}

static inline Thread* check_Thread(lua_State* ls, int arg) {
    return (Thread*)luaL_checkudata(ls, arg, mlua_Thread_name);
}

static void print_state(lua_State* ls) {
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &main_tag);

    printf("# Active:");
    lua_getupvalue(ls, -1, UV_TAIL);
    Thread* tail = to_Thread(ls, -1);
    if (tail == NULL) {
        printf("\n");
    } else {
        for (;;) {
            lua_getiuservalue(ls, -1, UV_NEXT);
            lua_remove(ls, -2);
            Thread* th = to_Thread(ls, -1);
            printf(" %p", th);
            if (th == tail) {
                printf("\n");
                break;
            }
        }
    }
    lua_pop(ls, 1);

    printf("# Timers:");
    lua_getupvalue(ls, -1, UV_TIMERS);
    for (;;) {
        Thread* th = to_Thread(ls, -1);
        if (th == NULL) break;
        printf(" %p (%" PRId64 ")", th, th->deadline);
        lua_getiuservalue(ls, -1, UV_NEXT);
        lua_remove(ls, -2);
    }
    printf("\n");
    lua_pop(ls, 2);
}

static void remove_timer(lua_State* ls, int thread, int main) {
    lua_getupvalue(ls, main, UV_TIMERS);  // prev = main.TIMERS
    if (lua_rawequal(ls, thread, -1)) {  // thread == prev?
        lua_pop(ls, 1);
        lua_getiuservalue(ls, thread, UV_NEXT);
        lua_setupvalue(ls, main, UV_TIMERS);  // main.TIMERS = thread.NEXT
        lua_pushnil(ls);
        lua_setiuservalue(ls, thread, UV_NEXT);  // thread.NEXT = nil
        return;
    }
    for (;;) {
        // next = prev.NEXT
        if (lua_getiuservalue(ls, -1, UV_NEXT) == LUA_TNIL) {
            lua_pop(ls, 2);
            return;
        }
        if (lua_rawequal(ls, -1, thread)) {  // next == thread?
            lua_pop(ls, 1);
            lua_getiuservalue(ls, thread, UV_NEXT);
            lua_setiuservalue(ls, -1, UV_NEXT);  // prev.NEXT = thread.NEXT
            lua_pop(ls, 1);
            lua_pushnil(ls);
            lua_setiuservalue(ls, thread, UV_NEXT);  // thread.NEXT = nil
            return;
        }
        lua_remove(ls, -2);  // prev = next
    }
}

static void main_remove_timer(lua_State* ls, int thread) {
    // thread == TIMERS?
    if (lua_rawequal(ls, thread, lua_upvalueindex(UV_TIMERS))) {
        lua_getiuservalue(ls, thread, UV_NEXT);
        lua_replace(ls, lua_upvalueindex(UV_TIMERS));  // TIMERS = thread.NEXT
        lua_pushnil(ls);
        lua_setiuservalue(ls, thread, UV_NEXT);  // thread.NEXT = nil
        return;
    }
    lua_pushvalue(ls, lua_upvalueindex(UV_TIMERS));  // prev = TIMERS
    for (;;) {
        // next = prev.NEXT
        if (lua_getiuservalue(ls, -1, UV_NEXT) == LUA_TNIL) {
            lua_pop(ls, 2);
            return;
        }
        if (lua_rawequal(ls, -1, thread)) {  // next == thread?
            lua_pop(ls, 1);
            lua_getiuservalue(ls, thread, UV_NEXT);
            lua_setiuservalue(ls, -1, UV_NEXT);  // prev.NEXT = thread.NEXT
            lua_pop(ls, 1);
            lua_pushnil(ls);
            lua_setiuservalue(ls, thread, UV_NEXT);  // thread.NEXT = nil
            return;
        }
        lua_remove(ls, -2);  // prev = next
    }
}

static void activate(lua_State* ls, Thread* th, int thread, int main) {
    th->deadline = DL_ACTIVE;
    lua_getupvalue(ls, main, UV_TAIL);
    // thread.NEXT = main.TAIL == nil ? thread : main.TAIL.NEXT
    if (lua_isnil(ls, -1)) {
        lua_pushvalue(ls, thread);
    } else {
        lua_getiuservalue(ls, -1, UV_NEXT);
        // main.TAIL.NEXT = thread
        lua_pushvalue(ls, thread);
        lua_setiuservalue(ls, -3, UV_NEXT);
    }
    lua_setiuservalue(ls, thread, UV_NEXT);
    lua_pop(ls, 1);  // Remove TAIL
    // main.TAIL = thread
    lua_pushvalue(ls, thread);
    lua_setupvalue(ls, main, UV_TAIL);
}

static void main_activate(lua_State* ls, Thread* th, int thread) {
    th->deadline = DL_ACTIVE;
    // thread.NEXT = TAIL == nil ? thread : TAIL.NEXT
    if (lua_isnil(ls, lua_upvalueindex(UV_TAIL))) {
        lua_pushvalue(ls, thread);
    } else {
        lua_getiuservalue(ls, lua_upvalueindex(UV_TAIL), UV_NEXT);
        // TAIL.NEXT = thread
        lua_pushvalue(ls, thread);
        lua_setiuservalue(ls, lua_upvalueindex(UV_TAIL), UV_NEXT);
    }
    lua_setiuservalue(ls, thread, UV_NEXT);
    // TAIL = thread
    lua_pushvalue(ls, thread);
    lua_replace(ls, lua_upvalueindex(UV_TAIL));
}

static void resume(lua_State* ls, int thread, int main) {
    Thread* th = to_Thread(ls, thread);
    if (th->deadline >= DL_ACTIVE) return;
    if (th->deadline != DL_INFINITE) remove_timer(ls, thread, main);
    activate(ls, th, thread, main);
}

static void main_resume(lua_State* ls, int thread) {
    Thread* th = to_Thread(ls, thread);
    if (th->deadline >= DL_ACTIVE) return;
    if (th->deadline != DL_INFINITE) main_remove_timer(ls, thread);
    main_activate(ls, th, thread);
}

static int Thread_name(lua_State* ls) {
    lua_pushliteral(ls, "TODO");
    return 1;
}

static int Thread_is_alive(lua_State* ls) {
    Thread* th = check_Thread(ls, 1);
    return lua_pushboolean(ls, th->deadline != DL_DEAD), 1;
}

static int Thread_is_waiting(lua_State* ls) {
    Thread* th = check_Thread(ls, 1);
    lua_pushboolean(ls, th->deadline != DL_ACTIVE && th->deadline != DL_DEAD);
    return 1;
}

static int Thread_resume(lua_State* ls) {
    Thread* self = check_Thread(ls, 1);
    lua_settop(ls, 1);
    if (self->deadline >= DL_ACTIVE) return lua_pushboolean(ls, false), 1;
    lua_getiuservalue(ls, 1, UV_MAIN);
    if (self->deadline != DL_INFINITE) remove_timer(ls, 1, 2);
    activate(ls, self, 1, 2);
    return lua_pushboolean(ls, true), 1;
}

static int Thread_kill(lua_State* ls) {
    Thread* self = check_Thread(ls, 1);
    lua_settop(ls, 1);

    // Check if the thread is trying to kill itself.
    lua_getiuservalue(ls, 1, UV_THREAD);
    lua_pushthread(ls);
    if (lua_rawequal(ls, -2, -1)) {
        return luaL_error(ls, "cannot close a running coroutine");  // TODO
    }
    lua_pop(ls, 2);

    // Remove the thread from the timer list if necessary.
    lua_getiuservalue(ls, 1, UV_MAIN);
    switch (self->deadline) {
    case DL_DEAD:  // Already dead, nothing to do
        lua_pushboolean(ls, false);
        return 1;
    case DL_ACTIVE:  // The thread will be removed from the active queue in main
        break;
    case DL_INFINITE:
        break;
    default:
        remove_timer(ls, 1, 2);
        break;
    }

    // Remove the thread from the main.THREADS.
    self->deadline = DL_DEAD;
    lua_getupvalue(ls, 2, UV_THREADS);
    lua_pushvalue(ls, 1);
    lua_pushnil(ls);
    lua_rawset(ls, -3);  // main.THREADS[thread] = nil
    lua_pop(ls, 1);  // Remove THREADS

    // Close the Lua thread and set the (potentially nil) error in THREAD.
    lua_getiuservalue(ls, 1, UV_THREAD);
    lua_State* ts = lua_tothread(ls, -1);
    lua_pop(ls, 1);
    if (lua_closethread(ts, ls) != LUA_OK) {
        lua_xmove(ts, ls, 1);
    } else {
        lua_pushnil(ls);
    }
    lua_setiuservalue(ls, 1, UV_THREAD);

    // Resume joiners.
    switch (lua_getiuservalue(ls, 1, UV_JOINERS)) {
    case LUA_TNONE:
    case LUA_TNIL:
        return lua_pushboolean(ls, true), 1;
    case LUA_TUSERDATA: {  // Resume one joiner
        resume(ls, lua_absindex(ls, -1), 2);
        lua_pop(ls, 1);  // Remove JOINERS
        break;
    }
    case LUA_TTABLE: {  // Resume multiple joiners
        lua_pushnil(ls);
        while (lua_next(ls, -2)) {
            lua_pop(ls, 1);  // Remove value
            resume(ls, lua_absindex(ls, -1), 2);
        }
        break;
    }}
    lua_pop(ls, 1);  // Remove JOINERS
    lua_pushnil(ls);
    lua_setiuservalue(ls, 1, UV_JOINERS);
    return lua_pushboolean(ls, true), 1;
}

static int Thread_join_1(lua_State* ls, int status, lua_KContext ctx);
static int Thread_join_2(lua_State* ls);

static int Thread_join(lua_State* ls) {
    // TODO: Remove from joiners list on exit
    Thread* self = check_Thread(ls, 1);
    if (self->deadline == DL_DEAD) return Thread_join_2(ls);

    // Add the running thread to self.JOINERS.
    lua_settop(ls, 1);
    // running = main.TAIL.NEXT
    lua_getiuservalue(ls, 1, UV_MAIN);
    lua_getupvalue(ls, 2, UV_TAIL);
    lua_getiuservalue(ls, -1, UV_NEXT);
    switch (lua_getiuservalue(ls, 1, UV_JOINERS)) {
    case LUA_TNONE:
    case LUA_TNIL:
        lua_pop(ls, 1);
        // self.JOINERS = running
        lua_setiuservalue(ls, 1, UV_JOINERS);
        break;
    case LUA_TUSERDATA:
        // self.JOINERS = {[self.JOINERS] = true, [running] = true}
        lua_createtable(ls, 0, 2);
        lua_rotate(ls, -3, 1);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -4);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -3);
        lua_setiuservalue(ls, 1, UV_JOINERS);
        break;
    case LUA_TTABLE:
        // self.JOINERS[running] = true
        lua_rotate(ls, -2, 1);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -3);
        break;
    }
    lua_settop(ls, 1);
    lua_pushboolean(ls, true);
    return mlua_event_yield(ls, 1, &Thread_join_1, 0);
}

static int Thread_join_1(lua_State* ls, int status, lua_KContext ctx) {
    Thread* self = to_Thread(ls, 1);
    if (self->deadline == DL_DEAD) return Thread_join_2(ls);
    lua_pushboolean(ls, true);
    return mlua_event_yield(ls, 1, &Thread_join_1, 0);
}

static int Thread_join_2(lua_State* ls) {
    if (lua_getiuservalue(ls, 1, UV_THREAD) != LUA_TNIL) lua_error(ls);
    return 0;
}

void mlua_thread_running(lua_State* ls) {
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &main_tag);
    lua_getupvalue(ls, -1, UV_TAIL);
    lua_getiuservalue(ls, -1, UV_NEXT);
    lua_remove(ls, -2);
}

static int mod_running(lua_State* ls) {
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &main_tag);
    lua_getupvalue(ls, -1, UV_TAIL);
    if (lua_isnil(ls, -1)) return 0;
    lua_getiuservalue(ls, -1, UV_NEXT);
    return 1;
}

static int mod_yield(lua_State* ls) {
    // TODO: Make yield yield 0 values => stay on active queue
    lua_settop(ls, 1);
    return lua_yield(ls, 1);
}

static int mod_suspend(lua_State* ls) {
    // TODO: Make suspend yield 1 value => suspend
    // TODO: Check argument type
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
    // TODO: arg 2 is the optional name

    // Create the thread object.
    Thread* th = lua_newuserdatauv(ls, sizeof(Thread), THREAD_UV_COUNT);
    luaL_getmetatable(ls, mlua_Thread_name);
    lua_setmetatable(ls, -2);
    // TODO: Set the thread name
    int thread = lua_gettop(ls);
    lua_State* ts = lua_newthread(ls);
    lua_pushvalue(ls, 1);
    lua_xmove(ls, ts, 1);
    lua_setiuservalue(ls, thread, UV_THREAD);  // thread.THREAD = ts
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &main_tag);
    lua_pushvalue(ls, -1);
    lua_setiuservalue(ls, thread, UV_MAIN);

    // Add it to main.THREADS.
    lua_getupvalue(ls, -1, UV_THREADS);
    lua_pushvalue(ls, thread);
    lua_pushboolean(ls, true);
    lua_rawset(ls, -3);  // main.THREADS[thread] = true
    lua_pop(ls, 1);  // Remove THREADS

    // Add the thread to the active queue.
    activate(ls, th, thread, lua_absindex(ls, -1));
    lua_pop(ls, 1);  // Remove main
    return 1;
}

static int mod_shutdown(lua_State* ls) {
    lua_settop(ls, 1);
    lua_pushnil(ls);
    lua_rotate(ls, 1, 1);
    return lua_yield(ls, 2);
}

static void close_thread(lua_State* ls, int arg) {
    lua_getiuservalue(ls, arg, UV_THREAD);
    lua_State* ts = lua_tothread(ls, -1);
    lua_pop(ls, 1);
    if (ts == NULL) return;
    lua_closethread(ts, ls);
}

static int main_done(lua_State* ls) {
    lua_settop(ls, 0);
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &main_tag);

    // Close all threads.
    lua_getupvalue(ls, 1, UV_THREADS);
    lua_pushnil(ls);
    while (lua_next(ls, -2)) {
        lua_pop(ls, 1);  // Remove value
        close_thread(ls, -1);
    }
    lua_pop(ls, 1);  // Remove THREADS

    // Clear main upvalues.
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
    bool reschedule_head = false;
    for (;;) {
        // Dispatch events.
        assert(lua_gettop(ls) == 1);  // main_done
        uint64_t min_ticks, deadline;
        mlua_ticks_range(&min_ticks, &deadline);
        Thread* timers = to_Thread(ls, lua_upvalueindex(UV_TIMERS));
        if (lua_type(ls, lua_upvalueindex(UV_TAIL)) != LUA_TNIL) {
            deadline = min_ticks;
        } else if (timers != NULL) {
            deadline = timers->deadline;
        }
        lua_pushcfunction(ls, &Thread_resume);  // TODO: Replace with C function call
        mlua_event_dispatch(ls, deadline, 2);
        lua_pop(ls, 1);  // Remove resume

        // Resume threads whose deadline has elapsed.
        assert(lua_gettop(ls) == 1);  // main_done
        uint64_t ticks = mlua_ticks();
        while (timers != NULL) {
            if (timers->deadline > ticks) break;
            timers->deadline = DL_ACTIVE;
            // thread = TIMERS
            lua_pushvalue(ls, lua_upvalueindex(UV_TIMERS));
            // TIMERS = TIMERS.NEXT
            lua_getiuservalue(ls, lua_upvalueindex(UV_TIMERS), UV_NEXT);
            lua_replace(ls, lua_upvalueindex(UV_TIMERS));
            timers = to_Thread(ls, lua_upvalueindex(UV_TIMERS));
            // thread.NEXT = TAIL == nil ? thread : TAIL.NEXT
            if (lua_isnil(ls, lua_upvalueindex(UV_TAIL))) {
                lua_pushvalue(ls, -1);
            } else {
                lua_getiuservalue(ls, lua_upvalueindex(UV_TAIL), UV_NEXT);
                // TAIL.NEXT = thread
                lua_pushvalue(ls, -2);
                lua_setiuservalue(ls, lua_upvalueindex(UV_TAIL), UV_NEXT);
            }
            lua_setiuservalue(ls, -2, UV_NEXT);
            // TAIL = thread
            lua_replace(ls, lua_upvalueindex(UV_TAIL));
        }

        // Skip scheduling if active queue is empty.
        assert(lua_gettop(ls) == 1);  // main_done
        if (lua_isnil(ls, lua_upvalueindex(UV_TAIL))) continue;

        // If the previous running thread is still active, move it to the end of
        // the active queue, after threads resumed by events.
        if (reschedule_head) {
            // TAIL = TAIL.NEXT
            lua_getiuservalue(ls, lua_upvalueindex(UV_TAIL), UV_NEXT);
            lua_replace(ls, lua_upvalueindex(UV_TAIL));
            reschedule_head = false;
        }

        // Get the head of the active queue; skip and remove dead threads.
        // head = TAIL.NEXT
        assert(lua_gettop(ls) == 1);  // main_done
        lua_getiuservalue(ls, lua_upvalueindex(UV_TAIL), UV_NEXT);
        Thread* head = to_Thread(ls, -1);
        bool empty = false;
        for (;;) {
            if (head->deadline == DL_ACTIVE) break;
            assert(head->deadline == DL_DEAD);
            // head == TAIL?
            if (lua_rawequal(ls, -1, lua_upvalueindex(UV_TAIL))) {
                lua_pop(ls, 1);  // Remove head
                // TAIL = nil
                lua_pushnil(ls);
                lua_replace(ls, lua_upvalueindex(UV_TAIL));
                empty = true;
                break;
            }
            // head = TAIL.NEXT = head.NEXT
            lua_getiuservalue(ls, -1, UV_NEXT);
            lua_copy(ls, -1, -2);
            lua_setiuservalue(ls, lua_upvalueindex(UV_TAIL), UV_NEXT);
            head = to_Thread(ls, -1);
        }
        if (empty) continue;

        // Resume the thread at the head of the active queue.
        assert(lua_gettop(ls) == 2);  // main_done, head
        lua_getiuservalue(ls, -1, UV_THREAD);
        lua_State* ts = lua_tothread(ls, -1);
        lua_pop(ls, 1);  // Remove head.THREAD
        int nres;
        if (lua_resume(ts, ls, 0, &nres) != LUA_YIELD) {
            head->deadline = DL_DEAD;
            // TAIL.NEXT = head.NEXT
            lua_getiuservalue(ls, -1, UV_NEXT);
            lua_setiuservalue(ls, lua_upvalueindex(UV_TAIL), UV_NEXT);

            // Close the Lua thread and set the (potentially nil) error in
            // head.THREAD.
            // lua_pop(ls, 1);
            if (lua_closethread(ts, ls) != LUA_OK) {
                lua_xmove(ts, ls, 1);
            } else {
                lua_pushnil(ls);
            }
            lua_setiuservalue(ls, -2, UV_THREAD);
            assert(lua_gettop(ls) == 2);  // main_done, head

            // Resume joiners.
            switch (lua_getiuservalue(ls, -1, UV_JOINERS)) {
            case LUA_TNONE:
            case LUA_TNIL:
                break;
            case LUA_TUSERDATA:  // Resume one joiner
                main_resume(ls, lua_absindex(ls, -1));
                break;
            case LUA_TTABLE:  // Resume multiple joiners
                lua_pushnil(ls);
                while (lua_next(ls, -2)) {
                    lua_pop(ls, 1);  // Remove value
                    main_resume(ls, lua_absindex(ls, -1));
                }
                break;
            }
            lua_pop(ls, 1);  // Remove JOINERS
            assert(lua_gettop(ls) == 2);  // main_done, head
            lua_pushnil(ls);
            lua_setiuservalue(ls, -2, UV_JOINERS);
            // THREADS[head] = nil
            lua_pushnil(ls);
            lua_rawset(ls, lua_upvalueindex(UV_THREADS));
            continue;
        }

        if (nres > 2) lua_pop(ts, nres - 2);
        if (nres == 2) {  // Shutdown
            printf("# Shutdown\n");
            lua_xmove(ts, ls, 1);
            return 1;
        }
        if (nres > 0) lua_xmove(ts, ls, nres);
        if (nres == 0 || !lua_toboolean(ls, -1)) {
            // Keep head in the active queue.
            lua_pop(ls, 1 + nres);
            reschedule_head = true;
            continue;
        }

        // Suspend head.
        assert(lua_gettop(ls) == 3);  // main_done, head, deadline
        // head.NEXT == head?
        lua_getiuservalue(ls, -2, UV_NEXT);
        if (lua_rawequal(ls, -1, -3)) {
            lua_pop(ls, 1);
            // TAIL = nil
            lua_pushnil(ls);
            lua_replace(ls, lua_upvalueindex(UV_TAIL));
        } else {
            // TAIL.NEXT = head.NEXT
            lua_setiuservalue(ls, lua_upvalueindex(UV_TAIL), UV_NEXT);
        }
        if (lua_type(ls, -1) == LUA_TBOOLEAN) {
            // Suspend head indefinitely.
            lua_pop(ls, 1);  // Remove deadline
            head->deadline = DL_INFINITE;
            lua_pushnil(ls);
            lua_setiuservalue(ls, -2, UV_NEXT);
            lua_pop(ls, 1);  // Remove head
            continue;
        }

        // Add head to timer list.
        head->deadline = mlua_check_int64(ls, -1);
        lua_pop(ls, 1);
        assert(lua_gettop(ls) == 2);  // main_done, head
        Thread* th = to_Thread(ls, lua_upvalueindex(UV_TIMERS));
        if (th == NULL || head->deadline < th->deadline) {
            // head.NEXT = TIMERS
            lua_pushvalue(ls, lua_upvalueindex(UV_TIMERS));
            lua_setiuservalue(ls, -2, UV_NEXT);
            // TIMERS = head
            lua_replace(ls, lua_upvalueindex(UV_TIMERS));
            continue;
        }
        // prev = TIMERS
        lua_pushvalue(ls, lua_upvalueindex(UV_TIMERS));
        for (;;) {
            lua_getiuservalue(ls, -1, UV_NEXT);  // next = prev.NEXT
            assert(lua_gettop(ls) == 4);  // main_done, head, prev, next
            Thread* th = to_Thread(ls, -1);
            if (th == NULL || head->deadline < th->deadline) {
                lua_setiuservalue(ls, -3, UV_NEXT);  // head.NEXT = next
                assert(lua_gettop(ls) == 3);  // main_done, head, prev
                lua_rotate(ls, -2, 1);
                lua_setiuservalue(ls, -2, UV_NEXT);  // prev.NEXT = head
                break;
            }
            lua_remove(ls, -2);  // prev = next
        }
        lua_pop(ls, 1);  // Remove prev
    }
}

MLUA_SYMBOLS(Thread_syms) = {
    MLUA_SYM_F(start, mod_),
    MLUA_SYM_F(shutdown, mod_),
    MLUA_SYM_F(name, Thread_),
    MLUA_SYM_F(is_alive, Thread_),
    MLUA_SYM_F(is_waiting, Thread_),
    MLUA_SYM_F(resume, Thread_),
    MLUA_SYM_F(kill, Thread_),
    MLUA_SYM_F(join, Thread_),
};

#define Thread___close Thread_join

MLUA_SYMBOLS_NOHASH(Thread_syms_nh) = {
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

MLUA_OPEN_MODULE(mlua.thread.c) {
    mlua_event_require(ls);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the Thread class.
    mlua_new_class(ls, mlua_Thread_name, Thread_syms, true);
    mlua_set_fields(ls, Thread_syms_nh);
    lua_pop(ls, 1);

    // Create the main() closure.
    lua_pushnil(ls);
    lua_pushnil(ls);
    lua_createtable(ls, 0, 0);
    lua_pushcclosure(ls, &mod_main, MAIN_UV_COUNT);
    lua_pushvalue(ls, -1);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, &main_tag);
    lua_setfield(ls, -2, "main");
    return 1;
}
