// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_HARDWARE_I2C_H
#define _MLUA_LIB_PICO_HARDWARE_I2C_H

#include "hardware/i2c.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/thread.h"

#ifdef __cplusplus
extern "C" {
#endif

// Get an i2c_inst_t* value at the given stack index, or raise an error if the
// stack entry is not an I2C userdata.
static inline i2c_inst_t* mlua_check_I2C(lua_State* ls, int arg) {
    extern char const mlua_I2C_name[];
    return *((i2c_inst_t**)luaL_checkudata(ls, arg, mlua_I2C_name));
}

// Get an i2c_inst_t* value at the given stack index.
static inline i2c_inst_t* mlua_to_I2C(lua_State* ls, int arg) {
    return *((i2c_inst_t**)lua_touserdata(ls, arg));
}

#if LIB_MLUA_MOD_MLUA_THREAD

typedef struct MLuaI2CState {
    MLuaEvent event;
} MLuaI2CState;

extern MLuaI2CState mlua_i2c_state[NUM_I2CS];

#endif  // LIB_MLUA_MOD_MLUA_THREAD

#ifdef __cplusplus
}
#endif

#endif
