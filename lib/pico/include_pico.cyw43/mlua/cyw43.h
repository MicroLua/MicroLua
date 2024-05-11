// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_PICO_CYW43_H
#define _MLUA_LIB_PICO_PICO_CYW43_H

#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

// Push fail, an error message, and an error code, and return the number of
// pushed values.
int mlua_cyw43_push_err(lua_State* ls, int err, char const* msg);

// If err == 0, push true, otherwise push fail, an error message and an error
// code. Returns the number of pushed values.
int mlua_cyw43_push_result(lua_State* ls, int err, char const* msg);

#ifdef __cplusplus
}
#endif

#endif
