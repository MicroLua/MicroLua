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

static int Buffer_ptr(lua_State* ls) {
    return lua_pushlightuserdata(ls, check_Buffer(ls, 1)), 1;
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
    MLUA_SYM_F(ptr, Buffer_),
};

MLUA_SYMBOLS_NOHASH(Buffer_syms_nh) = {
    MLUA_SYM_F_NH(__len, Buffer_),
    MLUA_SYM_F_NH(__buffer, Buffer_),
};

static void check_ro_buffer(lua_State* ls, int arg, MLuaBuffer* buf) {
    if (mlua_get_ro_buffer(ls, arg, buf)) return;
    luaL_typeerror(ls, arg, "string or buffer");
}

static lua_Unsigned optlen(lua_State* ls, MLuaBuffer const* buf, int arg,
                           lua_Unsigned def) {
    if (buf->size != (size_t)-1 && lua_isnoneornil(ls, arg)) return def;
    return luaL_checkinteger(ls, arg);
}

static inline void check_bounds(
        lua_State* ls, MLuaBuffer const* buf, lua_Unsigned off, int ioff,
        lua_Unsigned len, int ilen) {
    luaL_argcheck(ls, off <= buf->size, ioff, "out of bounds");
    luaL_argcheck(ls, off + len <= buf->size, ilen, "out of bounds");
}

static int mod_read(lua_State* ls) {
    MLuaBuffer src;
    check_ro_buffer(ls, 1, &src);
    lua_Unsigned off = luaL_optinteger(ls, 2, 0);
    lua_Unsigned len = optlen(ls, &src, 3, src.size - off);
    check_bounds(ls, &src, off, 2, len, 3);

    if (len == 0) return lua_pushliteral(ls, ""), 1;
    luaL_Buffer buf;
    void* dest = luaL_buffinitsize(ls, &buf, len);
    mlua_buffer_read(&src, off, len, dest);
    return luaL_pushresultsize(&buf, len), 1;
}

static int mod_read_cstr(lua_State* ls) {
    MLuaBuffer src;
    check_ro_buffer(ls, 1, &src);
    lua_Unsigned off = luaL_optinteger(ls, 2, 0);
    lua_Unsigned len = luaL_optinteger(ls, 3, src.size - off);
    check_bounds(ls, &src, off, 2, len, 3);

    uint8_t zero = 0;
    lua_Unsigned end = mlua_buffer_find(&src, off, len, &zero, 1);
    if (end != LUA_MAXUNSIGNED) len = end - off;
    luaL_Buffer buf;
    void* dest = luaL_buffinitsize(ls, &buf, len);
    mlua_buffer_read(&src, off, len, dest);
    return luaL_pushresultsize(&buf, len), 1;
}

static int mod_write(lua_State* ls) {
    MLuaBuffer dest;
    luaL_argexpected(ls, mlua_get_buffer(ls, 1, &dest), 1, "buffer");
    size_t len;
    void const* src = luaL_checklstring(ls, 2, &len);
    lua_Unsigned off = luaL_optinteger(ls, 3, 0);
    check_bounds(ls, &dest, off, 3, len, 2);

    mlua_buffer_write(&dest, off, len, src);
    return 0;
}

static int mod_fill(lua_State* ls) {
    MLuaBuffer dest;
    luaL_argexpected(ls, mlua_get_buffer(ls, 1, &dest), 1, "integer or buffer");
    int value = luaL_optinteger(ls, 2, 0);
    lua_Unsigned off = optlen(ls, &dest, 3, 0);
    lua_Unsigned len = luaL_optinteger(ls, 4, dest.size - off);
    check_bounds(ls, &dest, off, 3, len, 4);

    mlua_buffer_fill(&dest, off, len, value);
    return 0;
}

static int mod_find(lua_State* ls) {
    MLuaBuffer src;
    check_ro_buffer(ls, 1, &src);
    size_t needle_len;
    char const* needle = luaL_checklstring(ls, 2, &needle_len);
    lua_Unsigned off = luaL_optinteger(ls, 3, 0);
    lua_Unsigned len = luaL_optinteger(ls, 4, src.size - off);
    check_bounds(ls, &src, off, 3, len, 4);
    luaL_argcheck(ls, off + needle_len <= src.size, 2, "out of bounds");

    lua_Unsigned pos = mlua_buffer_find(&src, off, len, needle, needle_len);
    if (pos == LUA_MAXUNSIGNED) return 0;
    return lua_pushinteger(ls, pos), 1;
}

static int mod_get(lua_State* ls) {
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
    MLuaBuffer dest;
    luaL_argexpected(ls, mlua_get_buffer(ls, 1, &dest), 1, "integer or buffer");
    lua_Unsigned off = luaL_checkinteger(ls, 2);
    int top = lua_gettop(ls);
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
    MLUA_SYM_F(read_cstr, mod_),
    MLUA_SYM_F(write, mod_),
    MLUA_SYM_F(fill, mod_),
    MLUA_SYM_F(find, mod_),
    MLUA_SYM_F(get, mod_),
    MLUA_SYM_F(set, mod_),
    MLUA_SYM_F(alloc, mod_),
    MLUA_SYM_F(mallinfo, mod_),
};

MLUA_OPEN_MODULE(mlua.mem) {
    if (sizeof(size_t) > sizeof(lua_Integer)) {
        mlua_require(ls, "mlua.int64", false);
    }

    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the Buffer class.
    mlua_new_class(ls, Buffer_name, Buffer_syms, Buffer_syms_nh);
    lua_pop(ls, 1);
    return 1;
}
