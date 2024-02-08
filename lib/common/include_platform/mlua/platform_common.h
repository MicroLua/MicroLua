// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_PLATFORM_COMMON_H
#define _MLUA_LIB_COMMON_PLATFORM_COMMON_H

#include <stdint.h>

#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

// MLUA_IS64INT is true iff Lua is configured with 64-bit integers.
#define MLUA_IS64INT (((LUA_MAXINTEGER >> 31) >> 31) >= 1)

// A description of flash memory.
typedef struct MLuaFlash {
    uintptr_t address;
    uintptr_t size;
    uintptr_t write_size;
    uintptr_t erase_size;
} MLuaFlash;

#ifdef __cplusplus
}
#endif

#endif
