// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/event.h"

#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/platform.h"
#include "mlua/util.h"

// TODO: Combine enabling an event and watching an event. This might be tricky,
// as enabling has a locking function, but it might be doable
// TODO: Make yield status per-thread
// TODO: Add "performance" counters: dispatch cycles, sleeps

void mlua_event_require(lua_State* ls) {
    mlua_require(ls, "mlua.event", false);
}

void mlua_event_watch(lua_State* ls, MLuaEvent const* ev) {
    if (!mlua_event_enabled(ev)) {
        luaL_error(ls, "watching disabled event");
        return;
    }
    lua_pushthread(ls);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, ev);
}

void watch_from_thread(lua_State* ls, MLuaEvent const* ev, int thread) {
    if (!mlua_event_enabled(ev)) {
        luaL_error(ls, "watching disabled event");
        return;
    }
    lua_pushvalue(ls, thread);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, ev);
}

void mlua_event_unwatch(lua_State* ls, MLuaEvent const* ev) {
    if (!mlua_event_enabled(ev)) return;
    mlua_event_remove_watcher(ls, ev);
}

bool mlua_event_resume_watcher(lua_State* ls, MLuaEvent const* ev,
                               MLuaResume resume) {
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

int mlua_event_suspend(lua_State* ls, lua_KFunction cont, lua_KContext ctx,
                       int index) {
    if (index != 0) {
        lua_pushvalue(ls, index);
    } else {
        lua_pushboolean(ls, true);
    }
    return mlua_event_yield(ls, 1, cont, ctx);
}

static uint8_t yield_disabled;  // Just a marker for a registry entry

bool mlua_yield_enabled(lua_State* ls) {
    bool dis = lua_rawgetp(ls, LUA_REGISTRYINDEX, &yield_disabled) != LUA_TNIL;
    lua_pop(ls, 1);
    return !dis;
}

void mlua_set_yield_enabled(lua_State* ls, bool en) {
    if (en) {
        lua_pushnil(ls);
    } else {
        lua_pushboolean(ls, true);
    }
    lua_rawsetp(ls, LUA_REGISTRYINDEX, &yield_disabled);
}

bool mlua_event_can_wait(lua_State* ls, MLuaEvent const* ev) {
    return mlua_yield_enabled(ls) && mlua_event_enabled(ev);
}

static int mlua_event_loop_1(lua_State* ls, MLuaEvent const* ev,
                             MLuaEventLoopFn loop, int index);
static int mlua_event_loop_2(lua_State* ls, int status, lua_KContext ctx);

int mlua_event_loop(lua_State* ls, MLuaEvent const* ev, MLuaEventLoopFn loop,
                    int index) {
    if (!mlua_event_enabled(ev)) {
        return luaL_error(ls, "wait for disabled event");
    }
    int res = loop(ls, false);
    if (res >= 0) return res;
    if (index != 0) {
        if (lua_isnoneornil(ls, index)) {
            index = 0;
        } else {
            mlua_check_int64(ls, index);
        }
    }
    mlua_event_watch(ls, ev);
    return mlua_event_loop_1(ls, ev, loop, index);
}

static int mlua_event_loop_1(lua_State* ls, MLuaEvent const* ev,
                             MLuaEventLoopFn loop, int index) {
    lua_pushlightuserdata(ls, loop);
    lua_pushinteger(ls, index);
    return mlua_event_suspend(ls, &mlua_event_loop_2, (lua_KContext)ev, index);
}

static int mlua_event_loop_2(lua_State* ls, int status, lua_KContext ctx) {
    MLuaEventLoopFn loop = (MLuaEventLoopFn)lua_touserdata(ls, -2);
    int index = lua_tointeger(ls, -1);
    lua_pop(ls, 2);  // Restore the stack for loop
    int res = loop(ls, false);
    if (res < 0) {
        if (index == 0 ||
                !mlua_ticks_reached(mlua_to_int64(ls, index))) {
            return mlua_event_loop_1(ls, (MLuaEvent*)ctx, loop, index);
        }
        res = loop(ls, true);
    }
    mlua_event_unwatch(ls, (MLuaEvent const*)ctx);
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
    lua_callk(ls, 0, 0, 0, &handler_thread_2);
    return handler_thread_2(ls, LUA_OK, 0);
}

static int handler_thread_2(lua_State* ls, int status, lua_KContext ctx) {
    // Suspend until the event gets pending.
    lua_pushboolean(ls, true);
    return mlua_event_yield(ls, 1, &handler_thread_1, 0);
}

static int handler_thread_done(lua_State* ls) {
    // Stop watching the event.
    MLuaEvent* ev = lua_touserdata(ls, lua_upvalueindex(2));
    mlua_event_unwatch(ls, ev);

    // Call the "handler done" callback.
    if (!lua_isnil(ls, lua_upvalueindex(1))) {
        lua_pushvalue(ls, lua_upvalueindex(1));
        lua_callk(ls, 0, 0, 0, &mlua_cont_return_ctx);
    }
    return 0;
}

int mlua_event_handle(lua_State* ls, MLuaEvent* ev, lua_KFunction cont,
                      lua_KContext ctx) {
    lua_pushlightuserdata(ls, ev);
    lua_pushcclosure(ls, &handler_thread, 3);
    mlua_thread_start(ls);
    watch_from_thread(ls, ev, -1);
    // If the handler thread is killed before it gets a chance to run, it will
    // remain as a watcher and therefore leak. Since we yield here, this can
    // only happen from other threads that are on the active queue right now,
    // and only during this scheduling round. This is an unlikely sequence of
    // events, so we don't bother handling it.
    return mlua_event_yield(ls, 0, cont, ctx);
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

static bool resume_thread(lua_State* ls, lua_State* thread) {
    lua_pushvalue(ls, 1);
    lua_pushvalue(ls, -2);
    lua_call(ls, 1, 1);
    bool resumed = lua_toboolean(ls, -1);
    lua_pop(ls, 1);
    return resumed;
}

static int mod_dispatch(lua_State* ls) {
    uint64_t deadline = mlua_check_int64(ls, 1);
    lua_pushthread(ls);
    luaL_getmetafield(ls, -1, "resume");
    lua_replace(ls, 1);
    lua_settop(ls, 1);
    mlua_event_dispatch(ls, deadline, &resume_thread);
    return 0;
}

int mod_set_thread_metatable(lua_State* ls) {
    lua_pushthread(ls);
    lua_pushvalue(ls, 1);
    lua_setmetatable(ls, -2);
    return 0;
}

static int mod_yield_enabled(lua_State* ls) {
    bool en = mlua_yield_enabled(ls);
    if (!lua_isnoneornil(ls, 1)) {
        if (lua_toboolean(ls, 1)) {
            lua_pushnil(ls);
        } else {
            lua_pushboolean(ls, true);
        }
        lua_rawsetp(ls, LUA_REGISTRYINDEX, &yield_disabled);
    }
    return lua_pushboolean(ls, en), 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(dispatch, mod_),
    MLUA_SYM_F(set_thread_metatable, mod_),
    MLUA_SYM_F(yield_enabled, mod_),
};

MLUA_OPEN_MODULE(mlua.event) {
    mlua_require(ls, "mlua.int64", false);
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
