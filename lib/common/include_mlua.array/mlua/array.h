// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_MLUA_ARRAY_H
#define _MLUA_LIB_COMMON_MLUA_ARRAY_H

#include <stddef.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Handlers for a specific value type stored in an MLuaArray.
typedef struct MLuaValueType {
    void (*get)(lua_State*, void const*, size_t);
    void (*set)(lua_State*, int, void*, size_t);
} MLuaValueType;

// A fixed-capacity homogeneous array.
typedef struct MLuaArray {
    MLuaValueType const* type;
    void* data;
    size_t size;
    lua_Integer len;
    lua_Integer cap;
    uint64_t d64[0];
} MLuaArray;

#ifdef __cplusplus
}
#endif

#endif
