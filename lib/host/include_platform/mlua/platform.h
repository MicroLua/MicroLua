// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_HOST_PLATFORM_H
#define _MLUA_LIB_HOST_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"
#include "mlua/platform_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MLUA_HASH_SYMBOL_TABLES_DEFAULT 0

#define MLUA_PLATFORM_REGISTER_MODULE(n)

// Abort the program.
__attribute__((noreturn))
static inline void mlua_platform_abort(void) { abort(); }

// Perform set up at the very beginning of main().
static inline void mlua_platform_setup_main(int* argc, char* argv[]) {}

// Perform set up after creating an interpreter.
static inline void mlua_platform_setup_interpreter(lua_State* ls) {}

// Return the current microsecond ticks, as given by a monotonic clock.
uint64_t mlua_ticks64(void);

// Return the low-order bits of mlua_ticks64() that fit a lua_Unsigned.
lua_Unsigned mlua_ticks(void);

// Return true iff the given ticks64 value has been reached.
static inline bool mlua_ticks64_reached(uint64_t ticks) {
    return mlua_ticks64() >= ticks;
}

// Return true iff the given ticks value has been reached.
static inline bool mlua_ticks_reached(lua_Unsigned ticks) {
    return (mlua_ticks() - ticks) <= LUA_MAXINTEGER;
}

// Wait for an event, up to the given deadline. Returns true iff the deadline
// was reached.
bool mlua_wait(uint64_t deadline);

// Return a description of the flash memory of the platform, or NULL if the
// platform doesn't have flash memory.
static inline MLuaFlash const* mlua_platform_flash(void) { return NULL; }

#ifdef __cplusplus
}
#endif

#endif
