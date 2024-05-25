// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_PLATFORM_COMMON_H
#define _MLUA_LIB_COMMON_PLATFORM_COMMON_H

#include <stdint.h>

#include "lua.h"
#include "mlua/util.h"

#ifdef __cplusplus
extern "C" {
#endif

// The range of valid tick values.
#define MLUA_TICKS_MIN ((uint64_t)0)
#define MLUA_TICKS_MAX ((uint64_t)INT64_MAX)

// Return the absolute time corresponding to a timeout, relative to a time.
static inline uint64_t mlua_timeout_deadline(uint64_t time, uint64_t timeout) {
    uint64_t t = time + timeout;
    return t <= MLUA_TICKS_MAX ? t : MLUA_TICKS_MAX;
}

// Convert an integer-sized ticks value to a 64-bit ticks value.
static inline uint64_t mlua_to_ticks64(lua_Unsigned ticks, uint64_t now) {
    uint64_t res = (now & ~(uint64_t)LUA_MAXUNSIGNED) | ticks;
    lua_Unsigned t = now & LUA_MAXUNSIGNED;
    if (t < ticks && ticks - t > LUA_MAXINTEGER) {
        res -= ((uint64_t)LUA_MAXUNSIGNED + 1u);
    } else if (ticks < t && t - ticks > (lua_Unsigned)LUA_MININTEGER) {
        res += ((uint64_t)LUA_MAXUNSIGNED + 1u);
    }
    return res;
}

#if MLUA_IS64INT
// Ensure that the expression computing "now" is elided.
#define mlua_to_ticks64(ticks, now) ((uint64_t)(ticks))
#endif

// A description of flash memory.
typedef struct MLuaFlash {
    void const* ptr;
    size_t size;
    size_t write_size;
    size_t erase_size;
} MLuaFlash;

#ifdef __cplusplus
}
#endif

#endif
