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

static int buf_read(lua_State* ls, MLuaBuffer const* src, lua_Unsigned offset,
                    lua_Unsigned len) {
    if (len == 0) return lua_pushliteral(ls, ""), 1;
    luaL_Buffer buf;
    void* dest = (void*)luaL_buffinitsize(ls, &buf, len);
    if (src->vt != NULL) {
        src->vt->read(src->ptr, offset, len, dest);
    } else {
        memcpy(dest, src->ptr + offset, len);
    }
    return luaL_pushresultsize(&buf, len), 1;
}

static int mod_read(lua_State* ls) {
    MLuaBuffer buf = {.vt = NULL};
    int ok;
    buf.ptr = (void*)mlua_to_intptrx(ls, 1, &ok);
    if (ok) return buf_read(ls, &buf, 0, luaL_optinteger(ls, 2, 1));
    luaL_argexpected(ls, mlua_get_buffer(ls, 1, &buf), 1, "integer or buffer");
    lua_Unsigned offset = luaL_optinteger(ls, 2, 0);
    luaL_argcheck(ls, offset <= buf.size, 2, "out of bounds");
    lua_Unsigned len = luaL_optinteger(ls, 3, buf.size - offset);
    luaL_argcheck(ls, offset + len <= buf.size, 3, "out of bounds");
    return buf_read(ls, &buf, offset, len);
}

static int mod_write(lua_State* ls) {
    int ok;
    void* ptr = (void*)mlua_to_intptrx(ls, 1, &ok);
    size_t len;
    void const* src = luaL_checklstring(ls, 2, &len);
    if (ok) {
        memcpy(ptr, src, len);
        return 0;
    }
    MLuaBuffer buf;
    luaL_argexpected(ls, mlua_get_buffer(ls, 1, &buf), 1, "integer or buffer");
    lua_Unsigned offset = luaL_optinteger(ls, 3, 0);
    luaL_argcheck(ls, offset + len <= buf.size, 3, "out of bounds");
    if (buf.vt != NULL) {
        buf.vt->write(buf.ptr, offset, len, src);
    } else {
        memcpy(buf.ptr + offset, src, len);
    }
    return 0;
}

static int mod_fill(lua_State* ls) {
    int ok;
    void* ptr = (void*)mlua_to_intptrx(ls, 1, &ok);
    int value = luaL_optinteger(ls, 2, 0);
    if (ok) {
        memset(ptr, value, luaL_optinteger(ls, 3, 1));
        return 0;
    }
    MLuaBuffer buf;
    luaL_argexpected(ls, mlua_get_buffer(ls, 1, &buf), 1, "integer or buffer");
    lua_Unsigned offset = luaL_optinteger(ls, 3, 0);
    luaL_argcheck(ls, offset <= buf.size, 3, "out of bounds");
    lua_Unsigned len = luaL_optinteger(ls, 4, buf.size - offset);
    luaL_argcheck(ls, offset + len <= buf.size, 4, "out of bounds");
    if (buf.vt != NULL) {
        buf.vt->fill(buf.ptr, offset, len, value);
    } else {
        memset(buf.ptr + offset, value, len);
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
