// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/platform.h"

#include <time.h>

#ifdef CLOCK_BOOTTIME
#define CLOCK CLOCK_BOOTTIME
#else
#define CLOCK CLOCK_MONOTONIC
#endif

uint64_t mlua_ticks64(void) {
    struct timespec ts;
    clock_gettime(CLOCK, &ts);
    return (uint64_t)ts.tv_sec * 1000000u + ts.tv_nsec / 1000u;
}

lua_Unsigned mlua_ticks(void) {
    struct timespec ts;
    clock_gettime(CLOCK, &ts);
    return (lua_Unsigned)ts.tv_sec * 1000000u + ts.tv_nsec / 1000u;
}

bool mlua_wait(uint64_t deadline) {
    struct timespec ts = {.tv_sec = deadline / 1000000u,
                          .tv_nsec = (deadline % 1000000u) * 1000u};
    return clock_nanosleep(CLOCK, TIMER_ABSTIME, &ts, NULL) == 0;
}
