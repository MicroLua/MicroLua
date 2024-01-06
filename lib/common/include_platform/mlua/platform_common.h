// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_PLATFORM_COMMON_H
#define _MLUA_LIB_COMMON_PLATFORM_COMMON_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
