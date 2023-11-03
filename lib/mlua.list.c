// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

static char const list_name[] = "mlua.List";

#define LEN_IDX 0

static void new_list(lua_State* ls, int cap) {
    lua_createtable(ls, cap, 1);
    luaL_getmetatable(ls, list_name);
    lua_setmetatable(ls, -2);
}

static lua_Integer length(lua_State* ls, int index) {
    if (lua_isnoneornil(ls, index)) return 0;
    if (lua_rawgeti(ls, index, LEN_IDX) == LUA_TNIL) {
        lua_pop(ls, 1);
        return lua_rawlen(ls, index);
    }
    lua_Integer len = luaL_checkinteger(ls, -1);
    lua_pop(ls, 1);
    return len;
}

static int list_len(lua_State* ls) {
    if (lua_isnoneornil(ls, 1)) {
        lua_pushinteger(ls, 0);
    } else if (lua_rawgeti(ls, 1, LEN_IDX) == LUA_TNIL) {
        lua_pop(ls, 1);
        lua_pushinteger(ls, lua_rawlen(ls, 1));
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
    switch (lua_gettop(ls)) {
    case 0: lua_pushnil(ls);  // Fall through
    case 1: return 1;
    }
    lua_Integer len = 0;
    if (lua_isnil(ls, 1)) {
        new_list(ls, 0);
        lua_replace(ls, 1);
    } else {
        len = length(ls, 1);
    }
    int cnt = lua_gettop(ls) - 1;
    for (int i = cnt; i > 0; --i) lua_rawseti(ls, 1, luaL_intop(+, len, i));
    lua_pushinteger(ls, luaL_intop(+, len, cnt));
    lua_rawseti(ls, 1, LEN_IDX);
    return 1;
}

static int list_insert(lua_State* ls) {
    lua_Integer len = 0;
    if (lua_isnil(ls, 1)) {
        new_list(ls, 0);
        lua_replace(ls, 1);
    } else {
        len = length(ls, 1);
    }
    len = luaL_intop(+, len, 1);
    lua_Integer pos;
    switch (lua_gettop(ls)) {
    case 2:
        pos = len;
        break;
    case 3:
        pos = luaL_checkinteger(ls, 2);
        luaL_argcheck(ls, (lua_Unsigned)pos - 1u < (lua_Unsigned)len, 2,
                      "insert: position out of bounds");
        for (lua_Integer i = len; i > pos; --i) {
            lua_geti(ls, 1, i - 1);
            lua_seti(ls, 1, i);
        }
        break;
    default:
        return luaL_error(ls, "insert: wrong number of arguments");
    }
    lua_seti(ls, 1, pos);
    lua_pushinteger(ls, len);
    lua_rawseti(ls, 1, LEN_IDX);
    lua_settop(ls, 1);
    return 1;
}

static int list_remove(lua_State* ls) {
    lua_Integer len = length(ls, 1);
    lua_Integer pos = luaL_optinteger(ls, 2, len);
    if (pos != len) {
        luaL_argcheck(ls, (lua_Unsigned)pos - 1u <= (lua_Unsigned)len, 2,
                      "remove: position out of bounds");
    }
    if (len <= 0) return 0;
    lua_geti(ls, 1, pos);
    for (; pos < len; ++pos) {
        lua_geti(ls, 1, pos + 1);
        lua_seti(ls, 1, pos);
    }
    lua_pushnil(ls);
    lua_seti(ls, 1, pos);
    lua_pushinteger(ls, len - 1);
    lua_rawseti(ls, 1, LEN_IDX);
    return 1;
}

static int list_pack(lua_State* ls) {
    lua_Integer len = lua_gettop(ls);
    new_list(ls, len);
    lua_insert(ls, 1);
    for (lua_Integer i = len; i >= 1; --i) lua_rawseti(ls, 1, i);
    lua_pushinteger(ls, len);
    lua_rawseti(ls, 1, LEN_IDX);
    return 1;
}

static int list_unpack(lua_State* ls) {
    lua_Integer b = luaL_optinteger(ls, 2, 1);
    lua_Integer e = luaL_opt(ls, luaL_checkinteger, 3, length(ls, 1));
    if (b > e) return 0;
    lua_Integer n = e - b + 1;
    if (luai_unlikely(!lua_checkstack(ls, n))) {
        return luaL_error(ls, "unpack: too many results");
    }
    for (lua_Integer i = 0; i < n; ++i) lua_geti(ls, 1, b + i);
    return n;
}

static void add_value(lua_State* ls, luaL_Buffer* buf, lua_Integer i) {
    lua_geti(ls, 1, i);
    if (luai_unlikely(!lua_isstring(ls, -1))) {
        luaL_error(ls, "concat: invalid value (%s) at index %I",
                   luaL_typename(ls, -1), (LUAI_UACINT)i);
        return;
    }
    luaL_addvalue(buf);
}

static int list_concat(lua_State* ls) {
    lua_Integer last = length(ls, 1);
    size_t lsep;
    char const* sep = luaL_optlstring(ls, 2, "", &lsep);
    lua_Integer i = luaL_optinteger(ls, 3, 1);
    last = luaL_optinteger(ls, 4, last);

    luaL_Buffer buf;
    luaL_buffinit(ls, &buf);
    for (; i < last; ++i) {
        add_value(ls, &buf, i);
        luaL_addlstring(&buf, sep, lsep);
    }
    if (i == last) add_value(ls, &buf, i);
    luaL_pushresult(&buf);
    return 1;
}

static int list_find(lua_State* ls) {
    lua_Integer len = length(ls, 1);
    lua_Integer i = luaL_optinteger(ls, 3, 1);
    if (i <= -len) i = 1;
    else if (i < 0) i = len + i + 1;
    else if (i == 0) i = 1;
    for (; i <= len; ++i) {
        lua_geti(ls, 1, i);
        if (lua_compare(ls, 2, -1, LUA_OPEQ)) {
            lua_pushinteger(ls, i);
            return 1;
        }
    }
    return 0;
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
        lua_rawseti(ls, -2, LEN_IDX);
        return 1;
    }
    luaL_checktype(ls, 2, LUA_TTABLE);
    lua_settop(ls, 2);
    if (lua_rawgeti(ls, 2, LEN_IDX) == LUA_TNIL) {
        lua_pushinteger(ls, lua_rawlen(ls, 2));
        lua_rawseti(ls, 2, LEN_IDX);
    }
    lua_pop(ls, 1);
    luaL_getmetatable(ls, list_name);
    lua_setmetatable(ls, 2);
    return 1;
}

#define list___len list_len
#define list___eq list_eq

MLUA_SYMBOLS(list_syms) = {
    MLUA_SYM_F(len, list_),
    MLUA_SYM_F(eq, list_),
    MLUA_SYM_F(ipairs, list_),
    MLUA_SYM_F(append, list_),
    MLUA_SYM_F(insert, list_),
    MLUA_SYM_F(remove, list_),
    // TODO: MLUA_SYM_F(slice, list_),
    MLUA_SYM_F(pack, list_),
    MLUA_SYM_F(unpack, list_),
    // TODO: MLUA_SYM_F(sort, list_),
    // TODO: MLUA_SYM_F(move, list_),
    MLUA_SYM_F(concat, list_),
    MLUA_SYM_F(find, list_),
};

MLUA_SYMBOLS_NOHASH(list_syms_nh) = {
    MLUA_SYM_F_NH(__len, list_),
    MLUA_SYM_F_NH(__eq, list_),
    MLUA_SYM_F_NH(__repr, list_),
};

MLUA_SYMBOLS_NOHASH(list_meta_syms) = {
    MLUA_SYM_F_NH(__call, list_),
};

MLUA_OPEN_MODULE(mlua.list) {
    // Create the list class.
    mlua_new_class(ls, list_name, list_syms, false);
    mlua_set_fields(ls, list_syms_nh);
    mlua_set_meta_fields(ls, list_meta_syms);
    return 1;
}
