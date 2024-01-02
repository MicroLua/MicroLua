// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_PLATFORM_H
#define _MLUA_LIB_COMMON_PLATFORM_H

#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"
#include "mlua/platform_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

// Abort the program.
__attribute__((noreturn)) void mlua_platform_abort(void);

// Perform set up at the very beginning of main().
void mlua_platform_setup_main(int* argc, char* argv[]);

// Perform set up after creating an interpreter.
void mlua_platform_setup_interpreter(lua_State* ls);

// Return the current microsecond ticks, as given by a monotonic clock.
uint64_t mlua_platform_ticks_us(void);

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