// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/platform.h"

#include <stdbool.h>
#include <time.h>

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

bool mlua_wait(uint64_t deadline) {
    struct timespec ts = {.tv_sec = deadline / 1000000,
                          .tv_nsec = (deadline % 1000000) * 1000};
    return clock_nanosleep(CLOCK, TIMER_ABSTIME, &ts, NULL) == 0;
}
