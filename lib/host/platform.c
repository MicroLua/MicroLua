// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/platform.h"

#include "time.h"

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

MLuaFlash const* mlua_platform_flash(void) { return NULL }

uintptr_t mlua_platform_binary_size(void) { return 0; }
