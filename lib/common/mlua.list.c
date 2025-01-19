// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"
#include "mlua/util.h"

static char const list_name[] = "mlua.List";

#define LEN_IDX "n"

static void new_list(lua_State* ls, int cap) {
    lua_createtable(ls, cap, 1);
    luaL_getmetatable(ls, list_name);
    lua_setmetatable(ls, -2);
}

static lua_Integer length(lua_State* ls, int index) {
    if (lua_isnoneornil(ls, index)) return 0;
    if (lua_getfield(ls, index, LEN_IDX) == LUA_TNIL) {
        lua_pop(ls, 1);
        return lua_rawlen(ls, index);
    }
    lua_Integer len = luaL_checkinteger(ls, -1);
    lua_pop(ls, 1);
    return len;
}

static int list_len(lua_State* ls) {
    lua_Integer len = length(ls, 1);
    if (!lua_isnoneornil(ls, 2)) {
        lua_Integer new_len = luaL_checkinteger(ls, 2);
        if (new_len < 0) new_len = 0;
        if (new_len != len) {
            for (lua_Integer i = len; i > new_len; --i) {
                lua_pushnil(ls);
                lua_seti(ls, 1, i);
            }
            lua_pushinteger(ls, new_len);
            lua_setfield(ls, 1, LEN_IDX);
        }
    }
    return lua_pushinteger(ls, len), 1;
}

static int list___new(lua_State* ls) {
    lua_remove(ls, 1);  // Remove class
    if (lua_isnoneornil(ls, 1)) {
        new_list(ls, 0);
        lua_pushinteger(ls, 0);
        lua_setfield(ls, -2, LEN_IDX);
        return 1;
    }
    luaL_checktype(ls, 1, LUA_TTABLE);
    lua_settop(ls, 1);
    if (lua_getfield(ls, 1, LEN_IDX) == LUA_TNIL) {
        lua_pushinteger(ls, lua_rawlen(ls, 1));
        lua_setfield(ls, 1, LEN_IDX);
    }
    lua_pop(ls, 1);
    luaL_getmetatable(ls, list_name);
    lua_setmetatable(ls, 1);
    return 1;
}

static int list___len(lua_State* ls) {
    if (lua_isnoneornil(ls, 1)) return lua_pushinteger(ls, 0), 1;
    if (lua_getfield(ls, 1, LEN_IDX) == LUA_TNIL) {
        lua_pop(ls, 1);
        lua_pushinteger(ls, lua_rawlen(ls, 1));
    }
    return 1;
}

static int list___index2(lua_State* ls) { return 0; }

static int list___eq(lua_State* ls) {
    lua_Integer len1 = length(ls, 1), len2 = length(ls, 2);
    if (len1 != len2) return lua_pushboolean(ls, false), 1;
    for (lua_Integer i = 0; i < len1; ++i) {
        lua_Integer idx = i + 1;
        lua_geti(ls, 1, idx);
        lua_geti(ls, 2, idx);
        if (!mlua_compare_eq(ls, -2, -1)) return lua_pushboolean(ls, false), 1;
        lua_pop(ls, 2);
    }
    return lua_pushboolean(ls, true), 1;
}

static int ipairs_iter(lua_State* ls) {
    lua_Integer len = length(ls, 1);
    if (len == 0) return 0;
    lua_Integer i = luaL_checkinteger(ls, 2);
    if (i >= len) return 0;
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
    case 0: lua_pushnil(ls); __attribute__((fallthrough));
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
#if MLUA_APPEND_DESCENDING
    // Depending on Lua's allocation strategy for the array part of tables,
    // appending in descending order could be faster (allocate the full size
    // at once) or slower (switch to storing in hashed part). I haven't checked
    // yet which one it is, so in the meantime, we append in ascending order.
    for (int i = cnt; i > 0; --i) lua_seti(ls, 1, luaL_intop(+, len, i));
#else
    for (int i = 1; i <= cnt; ++i) {
        lua_pushvalue(ls, 1 + i);
        lua_seti(ls, 1, luaL_intop(+, len, i));
    }
    lua_settop(ls, 1);
#endif
    lua_pushinteger(ls, luaL_intop(+, len, cnt));
    lua_setfield(ls, 1, LEN_IDX);
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
                      "out of bounds");
        for (lua_Integer i = len; i > pos; --i) {
            lua_geti(ls, 1, i - 1);
            lua_seti(ls, 1, i);
        }
        break;
    default:
        return luaL_error(ls, "invalid arguments");
    }
    lua_seti(ls, 1, pos);
    lua_pushinteger(ls, len);
    lua_setfield(ls, 1, LEN_IDX);
    return lua_settop(ls, 1), 1;
}

static int list_remove(lua_State* ls) {
    lua_Integer len = length(ls, 1);
    lua_Integer pos = luaL_optinteger(ls, 2, len);
    if (pos != len) {
        luaL_argcheck(ls, (lua_Unsigned)pos - 1u <= (lua_Unsigned)len, 2,
                      "out of bounds");
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
    lua_setfield(ls, 1, LEN_IDX);
    return 1;
}

static int list_pack(lua_State* ls) {
    lua_Integer len = lua_gettop(ls);
    new_list(ls, len);
    lua_insert(ls, 1);
    for (lua_Integer i = len; i >= 1; --i) lua_rawseti(ls, 1, i);
    lua_pushinteger(ls, len);
    lua_setfield(ls, 1, LEN_IDX);
    return 1;
}

static int list_unpack(lua_State* ls) {
    lua_Integer b = luaL_optinteger(ls, 2, 1);
    lua_Integer e = luaL_opt(ls, luaL_checkinteger, 3, length(ls, 1));
    if (b > e) return 0;
    lua_Integer n = e - b + 1;
    if (luai_unlikely(!lua_checkstack(ls, n))) {
        return luaL_error(ls, "too many results");
    }
    for (lua_Integer i = 0; i < n; ++i) lua_geti(ls, 1, b + i);
    return n;
}

static int restore_mt(lua_State* ls) {
    lua_pushnil(ls);
    lua_setmetatable(ls, lua_upvalueindex(1));
    return 0;
}

static int list_sort(lua_State* ls) {
    if (lua_isnoneornil(ls, 1)) return lua_settop(ls, 1), 1;
    lua_settop(ls, 2);
    if (lua_getfield(ls, 1, LEN_IDX) != LUA_TNIL) {
        if (luaL_checkinteger(ls, -1) == 0) return lua_settop(ls, 1), 1;
        lua_pop(ls, 1);
        // The list has an explicit length but no metatable. Temporarily set
        // a metatable so that table.sort() gets the correct length, and remove
        // it on exit.
        if (lua_getmetatable(ls, 1)) {
            lua_pop(ls, 1);
        } else {
            lua_pushvalue(ls, 1);
            lua_pushcclosure(ls, &restore_mt, 1);
            lua_toclose(ls, -1);
            luaL_getmetatable(ls, list_name);
            lua_setmetatable(ls, 1);
        }
    } else {
        lua_pop(ls, 1);
    }
    lua_pushvalue(ls, lua_upvalueindex(1));
    lua_pushvalue(ls, 1);
    lua_pushvalue(ls, 2);
    lua_call(ls, 2, 0);
    return lua_settop(ls, 1), 1;
}

static void add_value(lua_State* ls, luaL_Buffer* buf, lua_Integer i) {
    lua_geti(ls, 1, i);
    if (luai_unlikely(!lua_isstring(ls, -1))) {
        luaL_error(ls, "invalid value (%s) at index %I", luaL_typename(ls, -1),
                   (LUAI_UACINT)i);
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
    return luaL_pushresult(&buf), 1;
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
            return lua_pushinteger(ls, i), 1;
        }
    }
    return 0;
}

static int repr_done(lua_State* ls) {
    lua_pushvalue(ls, lua_upvalueindex(1));
    lua_pushnil(ls);
    lua_rawset(ls, lua_upvalueindex(2));
    return 0;
}

static int list___repr(lua_State* ls) {
    lua_Integer len = length(ls, 1);
    if (len == 0) return lua_pushliteral(ls, "{}"), 1;

    // Set seen[self] = true and revert on exit.
    lua_pushvalue(ls, 1);
    lua_pushvalue(ls, 3);
    lua_pushcclosure(ls, &repr_done, 2);
    lua_toclose(ls, -1);
    lua_pushvalue(ls, 1);
    lua_pushboolean(ls, true);
    lua_rawset(ls, 3);

    luaL_Buffer buf;
    luaL_buffinit(ls, &buf);
    luaL_addchar(&buf, '{');
    for (lua_Integer i = 0; i < len; ++i) {
        if (i > 0) luaL_addstring(&buf, ", ");
        lua_pushvalue(ls, 2);  // repr
        lua_geti(ls, 1, i + 1);  // self[i + 1]
        lua_pushvalue(ls, 3);  // seen
        lua_call(ls, 2, 1);
        luaL_addvalue(&buf);
    }
    luaL_addchar(&buf, '}');
    return luaL_pushresult(&buf), 1;
}

#define list_eq list___eq

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
    // TODO: MLUA_SYM_F(move, list_),
    MLUA_SYM_F(concat, list_),
    MLUA_SYM_F(find, list_),
};

MLUA_SYMBOLS_NOHASH(list_syms_nh) = {
    MLUA_SYM_F_NH(__new, list_),
    MLUA_SYM_F_NH(__len, list_),
    MLUA_SYM_F_NH(__index2, list_),
    MLUA_SYM_F_NH(__eq, list_),
    MLUA_SYM_F_NH(__repr, list_),
    MLUA_SYM_V_NH(sort, boolean, false),  // Preallocate
};

MLUA_OPEN_MODULE(mlua.list) {
    mlua_require(ls, "table", true);

    // Create the list class.
    mlua_new_class(ls, list_name, list_syms, list_syms_nh);
    mlua_set_metaclass(ls);
    lua_getfield(ls, -2, "sort");
    lua_pushcclosure(ls, &list_sort, 1);
    lua_setfield(ls, -2, "sort");
    return 1;
}
