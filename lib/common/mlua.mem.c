// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <string.h>

#include "mlua/int64.h"
#include "mlua/mem.h"
#include "mlua/module.h"
#include "mlua/util.h"

// TODO: Rename Buffer to Block, and create a Buffer that includes a size
// TODO: Use Buffer for read operations (I2C, SPI, UART, stdio). Allow providing
//       a buffer when reading, so that it can be re-used.

static int read_mem(lua_State* ls, void const* src, size_t size) {
    if (size <= 0) {
        lua_pushliteral(ls, "");
        return 1;
    }
    luaL_Buffer buf;
    void* dest = (void*)luaL_buffinitsize(ls, &buf, size);
    memcpy(dest, src, size);
    luaL_pushresultsize(&buf, size);
    return 1;
}

char const mlua_Buffer_name[] = "mlua.mem.Buffer";

static int Buffer_addr(lua_State* ls) {
    mlua_push_intptr(ls, (uintptr_t)mlua_mem_check_Buffer(ls, 1));
    return 1;
}

static int Buffer_fill(lua_State* ls) {
    void* buf = mlua_mem_check_Buffer(ls, 1);
    lua_Unsigned size = lua_rawlen(ls, 1);
    int value = luaL_optinteger(ls, 2, 0);
    lua_Unsigned offset = luaL_optinteger(ls, 3, 0);
    luaL_argcheck(ls, offset <= size, 3, "out of bounds");
    lua_Unsigned len = luaL_optinteger(ls, 4, size - offset);
    luaL_argcheck(ls, offset + len <= size, 4, "out of bounds");
    memset(buf + offset, value, len);
    return lua_settop(ls, 1), 1;
}

static int Buffer_read(lua_State* ls) {
    void const* buf = mlua_mem_check_Buffer(ls, 1);
    lua_Unsigned size = lua_rawlen(ls, 1);
    lua_Unsigned offset = luaL_optinteger(ls, 2, 0);
    luaL_argcheck(ls, offset <= size, 2, "out of bounds");
    lua_Unsigned len = luaL_optinteger(ls, 3, size - offset);
    luaL_argcheck(ls, offset + len <= size, 3, "out of bounds");
    return read_mem(ls, buf + offset, len);
}

static int Buffer_write(lua_State* ls) {
    void* buf = mlua_mem_check_Buffer(ls, 1);
    lua_Unsigned size = lua_rawlen(ls, 1);
    size_t len;
    void const* src = (void const*)luaL_checklstring(ls, 2, &len);
    lua_Unsigned offset = luaL_optinteger(ls, 3, 0);
    luaL_argcheck(ls, offset + len <= size, 3, "out of bounds");
    memcpy(buf + offset, src, len);
    return 0;
}

static int Buffer___len(lua_State* ls) {
    lua_pushinteger(ls, lua_rawlen(ls, 1));
    return 1;
}

MLUA_SYMBOLS(Buffer_syms) = {
    MLUA_SYM_F(addr, Buffer_),
    MLUA_SYM_F(fill, Buffer_),
    MLUA_SYM_F(read, Buffer_),
    MLUA_SYM_F(write, Buffer_),
};

MLUA_SYMBOLS_NOHASH(Buffer_syms_nh) = {
    MLUA_SYM_F_NH(__len, Buffer_),
};

static int mod_read(lua_State* ls) {
    return read_mem(ls, (void const*)mlua_check_intptr(ls, 1),
                    luaL_optinteger(ls, 2, 1));
}

static int mod_write(lua_State* ls) {
    void* ptr = (void*)mlua_check_intptr(ls, 1);
    size_t len;
    void const* src = (void const*)luaL_checklstring(ls, 2, &len);
    memcpy(ptr, src, len);
    return 0;
}

static int mod_alloc(lua_State* ls) {
    lua_newuserdatauv(ls, luaL_checkinteger(ls, 1), 0);
    luaL_getmetatable(ls, mlua_Buffer_name);
    lua_setmetatable(ls, -2);
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(read, mod_),
    MLUA_SYM_F(write, mod_),
    MLUA_SYM_F(alloc, mod_),
};

MLUA_OPEN_MODULE(mlua.mem) {
    if (sizeof(intptr_t) > sizeof(lua_Integer)) {
        mlua_require(ls, "mlua.int64", false);
    }

    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the Buffer class.
    mlua_new_class(ls, mlua_Buffer_name, Buffer_syms, true);
    mlua_set_fields(ls, Buffer_syms_nh);
    lua_pop(ls, 1);
    return 1;
}
