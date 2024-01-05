// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/event.h"

#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/platform.h"
#include "mlua/util.h"

// TODO: Make some short functions inline
// TODO: Don't store the handler thread in the registry; it should already be
//       available as a (single) watcher; start watching in the parent
// TODO: Combine enabling an event and watching an event => always a single
//       watcher for each event. This might be tricky, as enabling has a locking
//       function, but it might be doable
// TODO: Add "performance" counters: dispatch cycles, sleeps

static inline void const* watchers_tag(MLuaEvent const* ev) { return ev; }

static inline void const* handler_tag(MLuaEvent const* ev) {
    return (void const*)ev + 1;
}

void mlua_event_require(lua_State* ls) {
    mlua_require(ls, "mlua.event", false);
}

void mlua_event_watch(lua_State* ls, MLuaEvent const* ev) {
    if (!mlua_event_enabled(ev)) {
        luaL_error(ls, "watching disabled event");
        return;
    }
    if (!lua_isyieldable(ls)) {
        luaL_error(ls, "watching event in unyieldable thread");
        return;
    }
    switch (lua_rawgetp(ls, LUA_REGISTRYINDEX, watchers_tag(ev))) {
    case LUA_TNIL:  // No watchers
        lua_pop(ls, 1);
        lua_pushthread(ls);
        lua_rawsetp(ls, LUA_REGISTRYINDEX, watchers_tag(ev));
        return;
    case LUA_TTHREAD:  // A single watcher
        lua_pushthread(ls);
        if (lua_rawequal(ls, -2, -1)) {  // Already registered
            lua_pop(ls, 2);
            return;
        }
        lua_createtable(ls, 0, 2);
        lua_rotate(ls, -2, 1);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -3);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -2);
        lua_rawsetp(ls, LUA_REGISTRYINDEX, watchers_tag(ev));
        return;
    case LUA_TTABLE:  // Multiple watchers
        lua_pushthread(ls);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -3);
        lua_pop(ls, 1);
        return;
    default:
        lua_pop(ls, 1);
        return;
    }
}

void mlua_event_unwatch(lua_State* ls, MLuaEvent const* ev) {
    if (!mlua_event_enabled(ev)) return;
    switch (lua_rawgetp(ls, LUA_REGISTRYINDEX, watchers_tag(ev))) {
    case LUA_TTHREAD:  // A single watcher
        lua_pushthread(ls);
        if (!lua_rawequal(ls, -2, -1)) {  // Not the current thread
            lua_pop(ls, 2);
            return;
        }
        lua_pop(ls, 2);
        lua_pushnil(ls);
        lua_rawsetp(ls, LUA_REGISTRYINDEX, watchers_tag(ev));
        return;
    case LUA_TTABLE:  // Multiple watchers
        lua_pushthread(ls);
        lua_pushnil(ls);
        lua_rawset(ls, -3);
        lua_pop(ls, 1);
        return;
    default:
        lua_pop(ls, 1);
        return;
    }
}

bool mlua_event_resume_watchers(lua_State* ls, MLuaEvent const* ev,
                                int resume) {
    bool resumed = false;
    switch (lua_rawgetp(ls, LUA_REGISTRYINDEX, watchers_tag(ev))) {
    case LUA_TTHREAD:  // A single watcher
        lua_pushvalue(ls, resume);
        lua_rotate(ls, -2, 1);
        lua_call(ls, 1, 1);
        resumed = lua_toboolean(ls, -1);
        break;
    case LUA_TTABLE:  // Multiple watchers
        lua_pushnil(ls);
        while (lua_next(ls, -2)) {
            lua_pop(ls, 1);
            lua_pushvalue(ls, resume);
            lua_pushvalue(ls, -2);
            lua_call(ls, 1, 1);
            resumed = resumed || lua_toboolean(ls, -1);
            lua_pop(ls, 1);
        }
        break;
    }
    lua_pop(ls, 1);
    return resumed;
}

void mlua_event_remove_watchers(lua_State* ls, MLuaEvent const* ev) {
    // TODO: Resume watchers so that they can exit
    lua_pushnil(ls);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, watchers_tag(ev));
}

int mlua_event_yield(lua_State* ls, int nresults, lua_KFunction cont,
                     lua_KContext ctx) {
    lua_yieldk(ls, nresults, ctx, cont);
    return cont(ls, LUA_OK, ctx);
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
                !mlua_platform_ticks_reached(mlua_to_int64(ls, index))) {
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

    // Watch the event.
    MLuaEvent* event = lua_touserdata(ls, lua_upvalueindex(3));
    mlua_event_watch(ls, event);
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
    lua_pushnil(ls);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, handler_tag(ev));

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
    lua_pushvalue(ls, -1);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, handler_tag(ev));
    return mlua_event_yield(ls, 0, cont, ctx);
}

void mlua_event_stop_handler(lua_State* ls, MLuaEvent const* ev) {
    if (lua_rawgetp(ls, LUA_REGISTRYINDEX, handler_tag(ev)) == LUA_TTHREAD) {
        mlua_thread_kill(ls);
    } else {
        lua_pop(ls, 1);
    }
}

int mlua_event_push_handler_thread(lua_State* ls, MLuaEvent const* ev) {
    return lua_rawgetp(ls, LUA_REGISTRYINDEX, handler_tag(ev));
}

static int mod_dispatch(lua_State* ls) {
    uint64_t deadline = mlua_check_int64(ls, 1);
    lua_pushthread(ls);
    luaL_getmetafield(ls, -1, "resume");
    lua_replace(ls, 1);
    lua_settop(ls, 1);
    return mlua_event_dispatch(ls, deadline, 1);
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
        mlua_set_yield_enabled(ls, lua_toboolean(ls, 1));
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
