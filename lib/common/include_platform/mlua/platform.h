// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_PLATFORM_H
#define _MLUA_LIB_COMMON_PLATFORM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Return the current time, as given by a monotonic clock.
uint64_t mlua_platform_time_us(void);

// Return the range of values that can be returned by mlua_platform_time_us().
void mlua_platform_time_range(uint64_t* min, uint64_t* max);

// A description of flash memory.
typedef struct MLuaFlash {
    uintptr_t address;
    uintptr_t size;
    uintptr_t write_size;
    uintptr_t erase_size;
} MLuaFlash;

// Return a description of the flash memory of the platform, or NULL if the
// platform doesn't have flash memory.
MLuaFlash const* mlua_platform_flash(void);

// Return the size of the binary.
uintptr_t mlua_platform_binary_size(void);

#ifdef __cplusplus
}
#endif

#endif
