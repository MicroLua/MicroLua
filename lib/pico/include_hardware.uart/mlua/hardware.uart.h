// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_HARDWARE_UART_H
#define _MLUA_LIB_PICO_HARDWARE_UART_H

#include "hardware/uart.h"

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Get a uart_inst_t* value at the given stack index, or raise an error if the
// stack entry is not a UART userdata.
static inline uart_inst_t* mlua_check_UART(lua_State* ls, int arg) {
    extern char const mlua_UART_name[];
    return *((uart_inst_t**)luaL_checkudata(ls, arg, mlua_UART_name));
}

#ifdef __cplusplus
}
#endif

#endif
