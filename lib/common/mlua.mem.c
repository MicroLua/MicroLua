// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <malloc.h>
#include <string.h>

#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/util.h"

// TODO: Use Buffer for read operations (I2C, SPI, UART, stdio). Allow providing
//       a buffer when reading, so that it can be re-used.

char const Buffer_name[] = "mlua.mem.Buffer";

static inline void* check_Buffer(lua_State* ls, int arg) {
    return luaL_checkudata(ls, arg, Buffer_name);
}

static int Buffer_addr(lua_State* ls) {
    return mlua_push_intptr(ls, (uintptr_t)check_Buffer(ls, 1)), 1;
}

static int Buffer___len(lua_State* ls) {
    return lua_pushinteger(ls, lua_rawlen(ls, 1)), 1;
}

static int Buffer___buffer(lua_State* ls) {
    lua_pushlightuserdata(ls, check_Buffer(ls, 1));
    lua_pushinteger(ls, lua_rawlen(ls, 1));
    return 2;
}

MLUA_SYMBOLS(Buffer_syms) = {
    MLUA_SYM_F(addr, Buffer_),
};

MLUA_SYMBOLS_NOHASH(Buffer_syms_nh) = {
    MLUA_SYM_F_NH(__len, Buffer_),
    MLUA_SYM_F_NH(__buffer, Buffer_),
};

static void check_ro_buffer(lua_State* ls, int arg, MLuaBuffer* buf) {
    size_t len;
    buf->ptr = (void*)lua_tolstring(ls, arg, &len);
    if (buf->ptr != NULL) {
        buf->vt = NULL;
        buf->size = len;
        return;
    }
    if (mlua_get_buffer(ls, arg, buf)) return;
    luaL_typeerror(ls, arg, "integer, string or buffer");
}

static inline void check_bounds(
        lua_State* ls, MLuaBuffer const* buf, lua_Unsigned off, int ioff,
        lua_Unsigned len, int ilen) {
    luaL_argcheck(ls, off <= buf->size, ioff, "out of bounds");
    luaL_argcheck(ls, off + len <= buf->size, ilen, "out of bounds");
}

static int mod_read(lua_State* ls) {
    int ok;
    void const* ptr = (void const*)mlua_to_intptrx(ls, 1, &ok);
    if (ok) return lua_pushlstring(ls, ptr, luaL_optinteger(ls, 2, 1)), 1;

    MLuaBuffer src;
    check_ro_buffer(ls, 1, &src);
    lua_Unsigned off = luaL_optinteger(ls, 2, 0);
    lua_Unsigned len = luaL_optinteger(ls, 3, src.size - off);
    check_bounds(ls, &src, off, 2, len, 3);

    if (len == 0) return lua_pushliteral(ls, ""), 1;
    luaL_Buffer buf;
    void* dest = luaL_buffinitsize(ls, &buf, len);
    mlua_buffer_read(&src, off, len, dest);
    return luaL_pushresultsize(&buf, len), 1;
}

static int mod_write(lua_State* ls) {
    int ok;
    void* ptr = (void*)mlua_to_intptrx(ls, 1, &ok);
    size_t len;
    void const* src = luaL_checklstring(ls, 2, &len);
    if (ok) return memcpy(ptr, src, len), 0;

    MLuaBuffer dest;
    luaL_argexpected(ls, mlua_get_buffer(ls, 1, &dest), 1, "integer or buffer");
    lua_Unsigned off = luaL_optinteger(ls, 3, 0);
    check_bounds(ls, &dest, off, 3, len, 2);

    mlua_buffer_write(&dest, off, len, src);
    return 0;
}

static int mod_fill(lua_State* ls) {
    int ok;
    void* ptr = (void*)mlua_to_intptrx(ls, 1, &ok);
    int value = luaL_optinteger(ls, 2, 0);
    if (ok) return memset(ptr, value, luaL_optinteger(ls, 3, 1)), 0;

    MLuaBuffer dest;
    luaL_argexpected(ls, mlua_get_buffer(ls, 1, &dest), 1, "integer or buffer");
    lua_Unsigned off = luaL_optinteger(ls, 3, 0);
    lua_Unsigned len = luaL_optinteger(ls, 4, dest.size - off);
    check_bounds(ls, &dest, off, 3, len, 4);

    mlua_buffer_fill(&dest, off, len, value);
    return 0;
}

static int mod_get(lua_State* ls) {
    int ok;
    uint8_t const* ptr = (uint8_t const*)mlua_to_intptrx(ls, 1, &ok);
    if (ok) {
        lua_Unsigned len = luaL_optinteger(ls, 2, 1);
        if (len <= 0) return 0;
        lua_settop(ls, 1);
        if (luai_unlikely(!lua_checkstack(ls, len))) {
            return luaL_error(ls, "too many results");
        }
        for (unsigned int i = 0; i < len; ++i, ++ptr) lua_pushinteger(ls, *ptr);
        return len;
    }

    MLuaBuffer src;
    check_ro_buffer(ls, 1, &src);
    lua_Unsigned off = luaL_checkinteger(ls, 2);
    lua_Unsigned len = luaL_optinteger(ls, 3, 1);
    check_bounds(ls, &src, off, 2, len, 3);

    if (len <= 0) return 0;
    lua_settop(ls, 1);
    if (luai_unlikely(!lua_checkstack(ls, len))) {
        return luaL_error(ls, "too many results");
    }
    for (lua_Unsigned i = 0; i < len; ++i, ++off) {
        uint8_t value;
        mlua_buffer_read(&src, off, 1, &value);
        lua_pushinteger(ls, value);
    }
    return len;
}

static int mod_set(lua_State* ls) {
    int top = lua_gettop(ls);
    int ok;
    uint8_t* ptr = (uint8_t*)mlua_to_intptrx(ls, 1, &ok);
    if (ok) {
        for (int i = 2; i <= top; ++i, ++ptr) *ptr = luaL_checkinteger(ls, i);
        return 0;
    }

    MLuaBuffer dest;
    luaL_argexpected(ls, mlua_get_buffer(ls, 1, &dest), 1, "integer or buffer");
    lua_Unsigned off = luaL_checkinteger(ls, 2);
    check_bounds(ls, &dest, off, 2, top - 2, 3 + (dest.size - off));

    for (int i = 3; i <= top; ++i, ++off) {
        uint8_t value = luaL_checkinteger(ls, i);
        mlua_buffer_write(&dest, off, 1, &value);
    }
    return 0;
}

static int mod_alloc(lua_State* ls) {
    lua_newuserdatauv(ls, luaL_checkinteger(ls, 1), 0);
    luaL_getmetatable(ls, Buffer_name);
    lua_setmetatable(ls, -2);
    return 1;
}

static int mod_mallinfo(lua_State* ls) {
#ifdef __GLIBC__
    struct mallinfo2 info = mallinfo2();
#else
    struct mallinfo info = mallinfo();
#endif
    mlua_push_size(ls, info.arena);
    mlua_push_size(ls, info.uordblks);
    return 2;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(read, mod_),
    MLUA_SYM_F(write, mod_),
    MLUA_SYM_F(fill, mod_),
    MLUA_SYM_F(get, mod_),
    MLUA_SYM_F(set, mod_),
    MLUA_SYM_F(alloc, mod_),
    MLUA_SYM_F(mallinfo, mod_),
};

MLUA_OPEN_MODULE(mlua.mem) {
    if (sizeof(intptr_t) > sizeof(lua_Integer)) {
        mlua_require(ls, "mlua.int64", false);
    }

    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the Buffer class.
    mlua_new_class(ls, Buffer_name, Buffer_syms, Buffer_syms_nh);
    lua_pop(ls, 1);
    return 1;
}
