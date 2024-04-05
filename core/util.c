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

bool mlua_opt_cbool(lua_State* ls, int arg, bool def) {
    return luaL_opt(ls, mlua_to_cbool, arg, def);
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

void* mlua_get_buffer(lua_State* ls, int arg, size_t* size) {
    if (luaL_getmetafield(ls, arg, "__buffer") == LUA_TNIL) return NULL;
    lua_pushvalue(ls, arg);
    lua_call(ls, 1, 2);
    void* ptr = lua_touserdata(ls, -2);
    luaL_argexpected(ls, ptr != NULL, -2, "userdata");
    *size = luaL_checkinteger(ls, -1);
    lua_pop(ls, 2);
    return ptr;
}

int mlua_push_fail(lua_State* ls, char const* err) {
    luaL_pushfail(ls);
    lua_pushstring(ls, err);
    return 2;
}

bool mlua_compare_eq(lua_State* ls, int arg1, int arg2) {
    int t1 = lua_type(ls, arg1), t2 = lua_type(ls, arg2);
    if (t1 == LUA_TNONE || t2 == LUA_TNONE) return false;
    if ((t1 == LUA_TUSERDATA) == (t2 == LUA_TUSERDATA)
            && (t1 == LUA_TTABLE) == (t2 == LUA_TTABLE)) {
        return lua_compare(ls, arg1, arg2, LUA_OPEQ);
    }
    arg1 = lua_absindex(ls, arg1);
    arg2 = lua_absindex(ls, arg2);
    if (luaL_getmetafield(ls, arg1, "__eq") != LUA_TNIL) {
        lua_pushvalue(ls, arg1);
        lua_pushvalue(ls, arg2);
    } else if (luaL_getmetafield(ls, arg2, "__eq") != LUA_TNIL) {
        lua_pushvalue(ls, arg2);
        lua_pushvalue(ls, arg1);
    } else {
        return false;
    }
    lua_call(ls, 2, 1);
    bool res = lua_toboolean(ls, -1);
    lua_pop(ls, 1);
    return res;
}
