// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/platform.h"

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

__attribute__((noreturn)) void mlua_platform_abort(void) { abort(); }

void mlua_platform_setup_main(int* argc, char* argv[]) {}

static int global_yield_enabled(lua_State* ls) {
    return lua_pushboolean(ls, false), 1;
}

void mlua_platform_setup_interpreter(lua_State* ls) {
    lua_pushcfunction(ls, &global_yield_enabled);
    lua_setglobal(ls, "yield_enabled");
}

#ifdef CLOCK_BOOTTIME
#define CLOCK CLOCK_BOOTTIME
#else
#define CLOCK CLOCK_MONOTONIC
#endif

uint64_t mlua_platform_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK, &ts);
    return ts.tv_sec * (uint64_t)1000000 + ts.tv_nsec / 1000;
}

void mlua_platform_time_range(uint64_t* min, uint64_t* max) {
    *min = 0;
    *max = INT64_MAX;
}

MLuaFlash const* mlua_platform_flash(void) { return NULL; }
uintptr_t mlua_platform_binary_size(void) { return 0; }
