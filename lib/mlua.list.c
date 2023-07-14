#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

static char const list_name[] = "list";

static void new_list(lua_State* ls, int len) {
    lua_createtable(ls, len, 1);
    luaL_getmetatable(ls, list_name);
    lua_setmetatable(ls, -2);
}

static lua_Integer length(lua_State* ls, int index) {
    if (lua_isnoneornil(ls, index)) return 0;
    if (lua_rawgeti(ls, index, 0) == LUA_TNIL) {
        lua_pop(ls, 1);
        return luaL_len(ls, index);
    }
    lua_Integer len = luaL_checkinteger(ls, -1);
    lua_pop(ls, 1);
    return len;
}

static int list_len(lua_State* ls) {
    if (lua_isnoneornil(ls, 1)) {
        lua_pushinteger(ls, 0);
    } else if (lua_rawgeti(ls, 1, 0) == LUA_TNIL) {
        lua_pop(ls, 1);
        lua_len(ls, 1);
    }
    return 1;
}

static int list_eq(lua_State* ls) {
    lua_Integer len1 = length(ls, 1);
    lua_Integer len2 = length(ls, 2);
    if (len1 != len2) {
        lua_pushboolean(ls, false);
        return 1;
    }
    for (lua_Integer i = 0; i < len1; ++i) {
        lua_Integer idx = i + 1;
        lua_geti(ls, 1, idx);
        lua_geti(ls, 2, idx);
        if (!lua_compare(ls, -2, -1, LUA_OPEQ)) {
            lua_pushboolean(ls, false);
            return 1;
        }
        lua_pop(ls, 2);
    }
    lua_pushboolean(ls, true);
    return 1;
}

static int ipairs_iter(lua_State* ls) {
    lua_Integer len = length(ls, 1);
    if (len == 0) return 0;
    lua_Integer i = luaL_checkinteger(ls, 2);
    if (i == len) return 0;
    i = luaL_intop(+, i, 1);
    lua_pushinteger(ls, i);
    lua_geti(ls, 1, i);
    return 2;
}

static int list_ipairs(lua_State* ls) {
    luaL_checkany(ls, 1);
    lua_pushcfunction(ls, &ipairs_iter);
    lua_pushvalue(ls, 1);
    lua_pushinteger(ls, 0);
    return 3;
}

static int list_append(lua_State* ls) {
    lua_Integer len = 0;
    if (lua_isnoneornil(ls, 1)) {
        new_list(ls, 0);
        lua_replace(ls, 1);
    } else {
        len = length(ls, 1);
    }
    int cnt = lua_gettop(ls) - 1;
    for (int i = cnt; i > 0; --i) lua_rawseti(ls, 1, len + i);
    lua_pushinteger(ls, len + cnt);
    lua_rawseti(ls, 1, 0);
    return 1;
}

static int list_pack(lua_State* ls) {
    lua_Integer len = lua_gettop(ls);
    new_list(ls, len);
    lua_insert(ls, 1);
    for (lua_Integer i = len; i >= 1; --i) lua_rawseti(ls, 1, i);
    lua_pushinteger(ls, len);
    lua_rawseti(ls, 1, 0);
    return 1;
}

static int list_unpack(lua_State* ls) {
    lua_Integer b = luaL_optinteger(ls, 2, 1);
    lua_Integer e = luaL_opt(ls, luaL_checkinteger, 3, length(ls, 1));
    if (b > e) return 0;
    lua_Integer n = e - b + 1;
    if (luai_unlikely(!lua_checkstack(ls, n))) {
        return luaL_error(ls, "too many results to unpack");
    }
    for (lua_Integer i = 0; i < n; ++i) lua_geti(ls, 1, b + i);
    return n;
}

static int list___repr(lua_State* ls) {
    luaL_Buffer buf;
    luaL_buffinit(ls, &buf);
    luaL_addchar(&buf, '{');
    lua_Integer len = length(ls, 1);
    for (lua_Integer i = 0; i < len; ++i) {
        if (i > 0) luaL_addstring(&buf, ", ");
        lua_pushvalue(ls, 2);
        lua_geti(ls, 1, i + 1);
        lua_call(ls, 1, 1);
        luaL_addvalue(&buf);
    }
    luaL_addchar(&buf, '}');
    luaL_pushresult(&buf);
    return 1;
}

static int list___call(lua_State* ls) {
    if (lua_isnoneornil(ls, 2)) {
        new_list(ls, 0);
        lua_pushinteger(ls, 0);
        lua_rawseti(ls, -2, 0);
        return 1;
    }
    lua_settop(ls, 2);
    if (lua_rawgeti(ls, 2, 0) == LUA_TNIL) {
        lua_pushinteger(ls, luaL_len(ls, 2));
        lua_rawseti(ls, 2, 0);
    }
    lua_pop(ls, 1);
    luaL_getmetatable(ls, list_name);
    lua_setmetatable(ls, 2);
    return 1;
}

#define list___len list_len
#define list___eq list_eq

static MLuaSym const list_syms[] = {
    MLUA_SYM_F(len, list_),
    MLUA_SYM_F(eq, list_),
    MLUA_SYM_F(ipairs, list_),
    MLUA_SYM_F(append, list_),
    MLUA_SYM_F(pack, list_),
    MLUA_SYM_F(unpack, list_),
    MLUA_SYM_F(__len, list_),
    MLUA_SYM_F(__eq, list_),
    MLUA_SYM_F(__repr, list_),
};

static MLuaSym const list_meta_syms[] = {
    MLUA_SYM_F(__call, list_),
};

int luaopen_mlua_list(lua_State* ls) {
    // Create the list class.
    mlua_new_class(ls, list_name, list_syms);
    mlua_new_table(ls, 0, list_meta_syms);
    lua_setmetatable(ls, -2);
    return 1;
}
