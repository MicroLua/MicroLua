// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_MLUA_THREAD_H
#define _MLUA_LIB_COMMON_MLUA_THREAD_H

#include <stdbool.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Require the mlua.thread module.
void mlua_thread_require(lua_State* ls);

#if !LIB_MLUA_MOD_MLUA_THREAD
#define mlua_thread_require(ls) do {} while(0)
#endif

// Return true iff blocking event handling is selected.
bool mlua_thread_blocking(lua_State* ls);

#if !LIB_MLUA_MOD_MLUA_THREAD
// TODO: Allow force-disabling blocking => eliminate blocking code
#define mlua_thread_blocking(ls) (0)
#endif

#ifdef __cplusplus
}
#endif

#endif
