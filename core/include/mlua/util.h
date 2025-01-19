// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_CORE_UTIL_H
#define _MLUA_CORE_UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MLUA_STR(n) #n
#define MLUA_ESTR(n) MLUA_STR(n)

// Return the number of elements in the given array.
#define MLUA_SIZE(a) (sizeof(a) / sizeof((a)[0]))

// Return a bit mask of n bits.
#define MLUA_MASK(n) ((1u << n) - 1)

// Return the number of trailing zero bits of the given value.
#define MLUA_CTZ(v) __builtin_ctz(v)

// Return the size of a field in a struct.
#define MLUA_SIZEOF_FIELD(t, m) sizeof(((t*)0)->m)

// MLUA_IS64INT is true iff Lua is configured with 64-bit integers.
#define MLUA_IS64INT (((LUA_MAXINTEGER >> 31) >> 31) >= 1)

// Call lua_callk(), then the continuation.
#define mlua_callk(ls, nargs, nres, k, ctx) \
    (lua_callk((ls), (nargs), (nres), (ctx), &k), k((ls), LUA_OK, (ctx)))

// Call lua_pcallk(), then the continuation.
#define mlua_pcallk(ls, nargs, nres, msgh, k, ctx) \
    (k((ls), lua_pcallk((ls), (nargs), (nres), (msgh), (ctx), &k), (ctx)))

// A continuation that returns its ctx argument.
static inline int mlua_cont_return(lua_State* ls, int status,
                                   lua_KContext ctx) {
    return ctx;
}

// A continuation that returns (top - ctx) values if the call was successful,
// or re-raises the error at the top of the stack.
int mlua_cont_return_results(lua_State* ls, int status, lua_KContext ctx);

// Load a module, and optionally keep a reference to it on the stack.
void mlua_require(lua_State* ls, char const* module, bool keep);

// Convert an argument to a boolean according to C rules: nil, false, 0, 0.0 and
// a missing argument are considered false, and everything else is true.
bool mlua_to_cbool(lua_State* ls, int arg);

// Convert an optional argument to a boolean according to C rules: false, 0 and
// 0.0 are considered false, and everything else is true. If the argument is
// absent or nil, return "def".
bool mlua_opt_cbool(lua_State* ls, int arg, bool def);

// Return the given argument as a userdata. Raises an error if the argument is
// not a userdata.
void* mlua_check_userdata(lua_State* ls, int arg);

// Return the given argument as a userdata, or NULL if the argument is nil.
// Raises an error if the argument is not a userdata or nil.
void* mlua_check_userdata_or_nil(lua_State* ls, int arg);

// A buffer vtable.
typedef struct MLuaBufferVt {
    void (*read)(void*, lua_Unsigned, lua_Unsigned, void*);
    void (*write)(void*, lua_Unsigned, lua_Unsigned, void const*);
    void (*fill)(void*, lua_Unsigned, lua_Unsigned, int);
    lua_Unsigned (*find)(void*, lua_Unsigned, lua_Unsigned, void const*,
                         lua_Unsigned);
} MLuaBufferVt;

// The parameters returned by the buffer protocol. When vt is NULL, the buffer
// is raw, i.e. a contiguous block of memory.
typedef struct MLuaBuffer {
    MLuaBufferVt const* vt;
    void* ptr;
    size_t size;
} MLuaBuffer;

// Apply the buffer protocol to the given argument, and return the buffer
// pointer and size.
bool mlua_get_buffer(lua_State* ls, int arg, MLuaBuffer* buf);

// Apply the buffer protocol to the given argument, and return the buffer
// pointer and size. Also accepts a string.
bool mlua_get_ro_buffer(lua_State* ls, int arg, MLuaBuffer* buf);

// Read from a buffer.
static inline void mlua_buffer_read(MLuaBuffer const* buf, lua_Unsigned off,
                                    lua_Unsigned len, void* dest) {
    if (buf->vt != NULL) {
        buf->vt->read(buf->ptr, off, len, dest);
    } else {
        memcpy(dest, buf->ptr + off, len);
    }
}

// Write to a buffer.
static inline void mlua_buffer_write(MLuaBuffer const* buf, lua_Unsigned off,
                                     lua_Unsigned len, void const* src) {
    if (buf->vt != NULL) {
        buf->vt->write(buf->ptr, off, len, src);
    } else {
        memcpy(buf->ptr + off, src, len);
    }
}

// Fill a part of a buffer.
static inline void mlua_buffer_fill(MLuaBuffer const* buf, lua_Unsigned off,
                                    lua_Unsigned len, int value) {
    if (buf->vt != NULL) {
        buf->vt->fill(buf->ptr, off, len, value);
    } else {
        memset(buf->ptr + off, value, len);
    }
}

// Find a substring of a buffer.
static inline lua_Unsigned mlua_buffer_find(
        MLuaBuffer const* buf, lua_Unsigned off, lua_Unsigned len,
        void const* needle, lua_Unsigned needle_len) {
    if (buf->vt != NULL) {
        return buf->vt->find(buf->ptr, off, len, needle, needle_len);
    } else {
        void const* pos = memmem(buf->ptr + off, len, needle, needle_len);
        return pos != NULL ? (lua_Unsigned)(pos - buf->ptr) : LUA_MAXUNSIGNED;
    }
}

// Push a failure and an error message, and return the number of pushed values.
int mlua_push_fail(lua_State* ls, char const* err);

// Return the result of comparing two values for equality, similar to
// lua_compare(..., LUA_OPEQ), but always call the __eq metamethod if one is
// available on either value.
bool mlua_compare_eq(lua_State* ls, int arg1, int arg2);

#ifdef __cplusplus
}
#endif

#endif
