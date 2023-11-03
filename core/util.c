// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/util.h"

#include <string.h>

#include "pico/platform.h"

spin_lock_t* mlua_lock;

int mlua_cont_return_ctx(lua_State* ls, int status, lua_KContext ctx) {
    return ctx;
}

void mlua_require(lua_State* ls, char const* module, bool keep) {
    lua_getglobal(ls, "require");
    lua_pushstring(ls, module);
    lua_call(ls, 1, keep ? 1 : 0);
}

bool mlua_to_cbool(lua_State* ls, int arg) {
    if (lua_isinteger(ls, arg)) return lua_tointeger(ls, arg) != 0;
    if (lua_type(ls, arg) == LUA_TNUMBER)
        return lua_tonumber(ls, arg) != l_mathop(0.0);
    return lua_toboolean(ls, arg);
}

void* mlua_check_userdata(lua_State* ls, int arg) {
    void* ud = lua_touserdata(ls, arg);
    luaL_argexpected(ls, ud != NULL, arg, "userdata");
    return ud;
}

void* mlua_check_userdata_or_nil(lua_State* ls, int arg) {
    if (lua_isnoneornil(ls, arg)) return NULL;
    void* ud = lua_touserdata(ls, arg);
    luaL_argexpected(ls, ud != NULL, arg, "userdata or nil");
    return ud;
}

#if LIB_MLUA_MOD_MLUA_EVENT

static bool yield_enabled[NUM_CORES];

bool mlua_yield_enabled(void) { return yield_enabled[get_core_num()]; }

#endif  // LIB_MLUA_MOD_MLUA_EVENT

static int global_yield_enabled(lua_State* ls) {
#if LIB_MLUA_MOD_MLUA_EVENT
    bool* en = &yield_enabled[get_core_num()];
    lua_pushboolean(ls, *en);
    if (!lua_isnoneornil(ls, 1)) *en = mlua_to_cbool(ls, 1);
#else
    lua_pushboolean(ls, false);
#endif
    return 1;
}

static int Function___close(lua_State* ls) {
    // Call the function itself, passing through the remaining arguments. This
    // makes to-be-closed functions the equivalent of deferreds.
    lua_callk(ls, lua_gettop(ls) - 1, 0, 0, &mlua_cont_return_ctx);
    return 0;
}

int mlua_thread_meta(lua_State* ls, char const* name) {
    lua_pushthread(ls);
    int res = luaL_getmetafield(ls, -1, name);
    lua_remove(ls, res != LUA_TNIL ? -2 : -1);
    return res;
}

void mlua_thread_start(lua_State* ls) {
    lua_pushthread(ls);
    if (luaL_getmetafield(ls, -1, "start") == LUA_TNIL) {
        luaL_error(ls, "threads have no start method");
        return;
    }
    lua_rotate(ls, -3, 1);
    lua_pop(ls, 1);
    lua_call(ls, 1, 1);
}

void mlua_thread_kill(lua_State* ls) {
    if (luaL_getmetafield(ls, -1, "kill") == LUA_TNIL) {
        luaL_error(ls, "threads have no kill method");
        return;
    }
    lua_rotate(ls, -2, 1);
    lua_call(ls, 1, 0);
}

bool mlua_thread_is_alive(lua_State* thread) {
    if (thread == NULL) return false;
    switch (lua_status(thread)) {
    case LUA_YIELD:
        return true;
    case LUA_OK: {
        lua_Debug ar;
        return lua_getstack(thread, 0, &ar) || lua_gettop(thread) != 0;
    }
    default:
        return false;
    }
}

static __attribute__((constructor)) void init(void) {
    mlua_lock = spin_lock_instance(next_striped_spin_lock_num());
#if LIB_MLUA_MOD_MLUA_EVENT
    for (uint core = 0; core < NUM_CORES; ++core) yield_enabled[core] = true;
#endif
}

void mlua_util_init(lua_State* ls) {
    // Set globals.
    lua_pushstring(ls, LUA_RELEASE);
    lua_setglobal(ls, "_RELEASE");
    lua_pushcfunction(ls, &global_yield_enabled);
    lua_setglobal(ls, "yield_enabled");
    // TODO: Add ipairs0() => iterate integer indexes from 0

    // Set a metatable on functions.
    lua_pushcfunction(ls, &Function___close);  // Any function will do
    lua_createtable(ls, 0, 1);
    lua_pushcfunction(ls, &Function___close);
    lua_setfield(ls, -2, "__close");
    lua_setmetatable(ls, -2);
    lua_pop(ls, 1);

    // Load the mlua module and register it in globals.
    static char const mlua_name[] = "mlua";
    mlua_require(ls, mlua_name, true);
    lua_setglobal(ls, mlua_name);
}
