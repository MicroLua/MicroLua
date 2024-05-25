// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/address_mapped.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

static io_rw_8* check_io_rw_8(lua_State* ls, int arg) {
    luaL_argexpected(ls, lua_islightuserdata(ls, arg), arg, "pointer");
    return lua_touserdata(ls, arg);
}

static io_rw_16* check_io_rw_16(lua_State* ls, int arg) {
    luaL_argexpected(ls, lua_islightuserdata(ls, arg), arg, "pointer");
    io_rw_16* ptr = lua_touserdata(ls, arg);
    luaL_argcheck(ls, ((uintptr_t)ptr & 0x1) == 0, arg, "not 16-bit aligned");
    return ptr;
}

static io_rw_32* check_io_rw_32(lua_State* ls, int arg) {
    luaL_argexpected(ls, lua_islightuserdata(ls, arg), arg, "pointer");
    io_rw_32* ptr = lua_touserdata(ls, arg);
    luaL_argcheck(ls, ((uintptr_t)ptr & 0x3) == 0, arg, "not 32-bit aligned");
    return ptr;
}

#define READ_FN(bits) \
static int mod_read ## bits(lua_State* ls) { \
    io_rw_ ## bits* ptr = check_io_rw_ ## bits(ls, 1); \
    int count = luaL_optinteger(ls, 2, 1); \
    if (count <= 0) return 0; \
    lua_settop(ls, 0); \
    luaL_checkstack(ls, count, "too many results"); \
    for (int i = 0; i < count; ++i) lua_pushinteger(ls, *ptr++); \
    return count; \
}

READ_FN(8)
READ_FN(16)
READ_FN(32)

#define WRITE_FN(bits) \
static int mod_write ## bits(lua_State* ls) { \
    io_rw_ ## bits* ptr = check_io_rw_ ## bits(ls, 1); \
    int top = lua_gettop(ls); \
    for (int i = 2; i <= top; ++i) *ptr++ = luaL_checkinteger(ls, i); \
    return 0; \
}

WRITE_FN(8)
WRITE_FN(16)
WRITE_FN(32)

MLUA_FUNC_V2(mod_,, hw_set_bits, check_io_rw_32, luaL_checkinteger)
MLUA_FUNC_V2(mod_,, hw_clear_bits, check_io_rw_32, luaL_checkinteger)
MLUA_FUNC_V2(mod_,, hw_xor_bits, check_io_rw_32, luaL_checkinteger)
MLUA_FUNC_V3(mod_,, hw_write_masked, check_io_rw_32, luaL_checkinteger,
             luaL_checkinteger)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(read8, mod_),
    MLUA_SYM_F(read16, mod_),
    MLUA_SYM_F(read32, mod_),
    MLUA_SYM_F(write8, mod_),
    MLUA_SYM_F(write16, mod_),
    MLUA_SYM_F(write32, mod_),
    MLUA_SYM_F(hw_set_bits, mod_),
    MLUA_SYM_F(hw_clear_bits, mod_),
    MLUA_SYM_F(hw_xor_bits, mod_),
    MLUA_SYM_F(hw_write_masked, mod_),
};

MLUA_OPEN_MODULE(hardware.base) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
