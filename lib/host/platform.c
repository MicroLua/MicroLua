// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/platform.h"

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

__attribute__((noreturn)) void mlua_platform_abort(void) { abort(); }

void mlua_platform_setup_main(int* argc, char* argv[]) {}
void mlua_platform_setup_interpreter(lua_State* ls) {}

void mlua_ticks_range(uint64_t* min, uint64_t* max) {
    *min = 0;
    *max = INT64_MAX;
}

#ifdef CLOCK_BOOTTIME
#define CLOCK CLOCK_BOOTTIME
#else
#define CLOCK CLOCK_MONOTONIC
#endif

uint64_t mlua_ticks(void) {
    struct timespec ts;
    clock_gettime(CLOCK, &ts);
    return ts.tv_sec * (uint64_t)1000000 + ts.tv_nsec / 1000;
}

bool mlua_ticks_reached(uint64_t ticks) {
    return mlua_ticks() >= ticks;
}

bool mlua_wait(uint64_t deadline) {
    struct timespec ts = {.tv_sec = deadline / 1000000,
                          .tv_nsec = (deadline % 1000000) * 1000};
    return clock_nanosleep(CLOCK, TIMER_ABSTIME, &ts, NULL) == 0;
}

MLuaFlash const* mlua_platform_flash(void) { return NULL; }
uintptr_t mlua_platform_binary_size(void) { return 0; }
