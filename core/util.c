// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/util.h"

#include <string.h>

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
    if (lua_type(ls, arg) == LUA_TNUMBER) {
        return lua_tonumber(ls, arg) != l_mathop(0.0);
    }
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

int mlua_push_fail(lua_State* ls, char const* err) {
    luaL_pushfail(ls);
    lua_pushstring(ls, err);
    return 2;
}

static int Function___close(lua_State* ls) {
    // Call the function itself, passing through the remaining arguments. This
    // makes to-be-closed functions the equivalent of deferreds.
    lua_callk(ls, lua_gettop(ls) - 1, 0, 0, &mlua_cont_return_ctx);
    return 0;
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
    lua_call(ls, 1, 1);
}
