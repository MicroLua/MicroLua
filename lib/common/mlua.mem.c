// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

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

static char const Buffer_name[] = "mlua.mem.Buffer";

static inline void* check_Buffer(lua_State* ls, int arg) {
    return luaL_checkudata(ls, arg, Buffer_name);
}

static int Buffer_addr(lua_State* ls) {
    lua_pushinteger(ls, (uintptr_t)check_Buffer(ls, 1));
    return 1;
}

static int Buffer_clear(lua_State* ls) {
    void* buf = check_Buffer(ls, 1);
    lua_Unsigned size = lua_rawlen(ls, 1);
    memset(buf, luaL_optinteger(ls, 2, 0), size);
    return 0;
}

static int Buffer_read(lua_State* ls) {
    void const* buf = check_Buffer(ls, 1);
    lua_Unsigned size = lua_rawlen(ls, 1);
    lua_Unsigned offset = luaL_optinteger(ls, 2, 0);
    luaL_argcheck(ls, offset <= size, 2, "out of bounds");
    lua_Unsigned len = luaL_optinteger(ls, 3, size - offset);
    luaL_argcheck(ls, offset + len <= size, 3, "out of bounds");
    return read_mem(ls, buf + offset, len);
}

static int Buffer_write(lua_State* ls) {
    void* buf = check_Buffer(ls, 1);
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
    MLUA_SYM_F(clear, Buffer_),
    MLUA_SYM_F(read, Buffer_),
    MLUA_SYM_F(write, Buffer_),
};

MLUA_SYMBOLS_NOHASH(Buffer_syms_nh) = {
    MLUA_SYM_F_NH(__len, Buffer_),
};

static int mod_read(lua_State* ls) {
    if (sizeof(void*) > sizeof(lua_Integer)) {
        return luaL_error(ls, "not supported");
    }
    return read_mem(ls, (void const*)(uintptr_t)luaL_checkinteger(ls, 1),
                    luaL_optinteger(ls, 2, 1));
}

static int mod_write(lua_State* ls) {
    if (sizeof(void*) > sizeof(lua_Integer)) {
        return luaL_error(ls, "not supported");
    }
    void* ptr = (void*)(uintptr_t)luaL_checkinteger(ls, 1);
    size_t len;
    void const* src = (void const*)luaL_checklstring(ls, 2, &len);
    memcpy(ptr, src, len);
    return 0;
}

static int mod_alloc(lua_State* ls) {
    lua_newuserdatauv(ls, luaL_checkinteger(ls, 1), 0);
    luaL_getmetatable(ls, Buffer_name);
    lua_setmetatable(ls, -2);
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(read, mod_),
    MLUA_SYM_F(write, mod_),
    MLUA_SYM_F(alloc, mod_),
};

MLUA_OPEN_MODULE(mlua.mem) {
    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the Buffer class.
    mlua_new_class(ls, Buffer_name, Buffer_syms, true);
    mlua_set_fields(ls, Buffer_syms_nh);
    lua_pop(ls, 1);
    return 1;
}
