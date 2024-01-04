// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_PLATFORM_DEFS_H
#define _MLUA_LIB_PICO_PLATFORM_DEFS_H

#include "pico/binary_info.h"
#include "pico/platform.h"

#define MLUA_HASH_SYMBOL_TABLES_DEFAULT 1

#define MLUA_BI_TAG BINARY_INFO_MAKE_TAG('M', 'L')
#define MLUA_BI_FROZEN_MODULE 0xcb9305cf

#define MLUA_PLATFORM_REGISTER_MODULE(n) \
    bi_decl(bi_string(MLUA_BI_TAG, MLUA_BI_FROZEN_MODULE, #n))

#define MLUA_TIME_CRITICAL(fn) __time_critical_func(fn)

#endif
