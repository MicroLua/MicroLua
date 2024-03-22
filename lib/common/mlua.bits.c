// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "lua.h"
#include "lauxlib.h"
#include "mlua/int64.h"
#include "mlua/module.h"

#if MLUA_IS64INT
#define CLZ __builtin_clzll
#define CTZ __builtin_ctzll
#define POPCOUNT __builtin_popcountll
#define PARITY __builtin_parityll
#else
#define CLZ __builtin_clz
#define CTZ __builtin_ctz
#define POPCOUNT __builtin_popcount
#define PARITY __builtin_parity
#endif

static int mod_leading_zeros(lua_State* ls) {
    if (luai_likely(lua_isinteger(ls, 1))) {
        return lua_pushinteger(ls, CLZ(lua_tointeger(ls, 1))), 1;
    }
#if !MLUA_IS64INT
    int64_t v;
    if (mlua_test_int64(ls, 1, &v)) {
        return lua_pushinteger(ls, __builtin_clzll(v)), 1;
    }
#endif
    return luaL_typeerror(ls, 1, "integer or Int64");
}

static int mod_trailing_zeros(lua_State* ls) {
    if (luai_likely(lua_isinteger(ls, 1))) {
        return lua_pushinteger(ls, CTZ(lua_tointeger(ls, 1))), 1;
    }
#if !MLUA_IS64INT
    int64_t v;
    if (mlua_test_int64(ls, 1, &v)) {
        return lua_pushinteger(ls, __builtin_ctzll(v)), 1;
    }
#endif
    return luaL_typeerror(ls, 1, "integer or Int64");
}

static int mod_ones(lua_State* ls) {
    if (luai_likely(lua_isinteger(ls, 1))) {
        return lua_pushinteger(ls, POPCOUNT(lua_tointeger(ls, 1))), 1;
    }
#if !MLUA_IS64INT
    int64_t v;
    if (mlua_test_int64(ls, 1, &v)) {
        return lua_pushinteger(ls, __builtin_popcountll(v)), 1;
    }
#endif
    return luaL_typeerror(ls, 1, "integer or Int64");
}

static int mod_parity(lua_State* ls) {
    if (luai_likely(lua_isinteger(ls, 1))) {
        return lua_pushinteger(ls, PARITY(lua_tointeger(ls, 1))), 1;
    }
#if !MLUA_IS64INT
    int64_t v;
    if (mlua_test_int64(ls, 1, &v)) {
        return lua_pushinteger(ls, __builtin_parityll(v)), 1;
    }
#endif
    return luaL_typeerror(ls, 1, "integer or Int64");
}

static int mod_mask(lua_State* ls) {
    lua_Unsigned bits = luaL_checkinteger(ls, 1);
    if (bits < sizeof(lua_Unsigned) * 8) {
        return lua_pushinteger(ls, ((lua_Unsigned)1u << bits) - 1), 1;
    }
    if (bits == sizeof(lua_Unsigned) * 8) {
        return lua_pushinteger(ls, ~(lua_Unsigned)0), 1;
    }
#if !MLUA_IS64INT
    if (bits < sizeof(uint64_t) * 8) {
        return mlua_push_int64(ls, ((uint64_t)1u << bits) - 1), 1;
    }
    if (bits == sizeof(uint64_t) * 8) {
        return mlua_push_int64(ls, ~(uint64_t)0), 1;
    }
#endif
    return luaL_argerror(ls, 1, "too large");
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(leading_zeros, mod_),
    MLUA_SYM_F(trailing_zeros, mod_),
    MLUA_SYM_F(ones, mod_),
    MLUA_SYM_F(parity, mod_),
    MLUA_SYM_F(mask, mod_),
};

MLUA_OPEN_MODULE(mlua.bits) {
    mlua_require(ls, "mlua.int64", false);
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
