// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_HARDWARE_GPIO_H
#define _MLUA_LIB_PICO_HARDWARE_GPIO_H

#include "pico.h"

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Return the given argument as a GPIO number. Raises an error if the argument
// value is out of bounds.
static inline uint mlua_check_gpio(lua_State* ls, int arg) {
    lua_Unsigned num = luaL_checkinteger(ls, arg);
    luaL_argcheck(ls, num < NUM_BANK0_GPIOS, arg, "invalid GPIO");
    return num;
}

#ifdef __cplusplus
}
#endif

#endif
