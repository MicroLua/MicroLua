// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/vreg.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

MLUA_FUNC_V1(mod_, vreg_, set_voltage, luaL_checkinteger)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(VOLTAGE_0_85, integer, VREG_VOLTAGE_0_85),
    MLUA_SYM_V(VOLTAGE_0_90, integer, VREG_VOLTAGE_0_90),
    MLUA_SYM_V(VOLTAGE_0_95, integer, VREG_VOLTAGE_0_95),
    MLUA_SYM_V(VOLTAGE_1_00, integer, VREG_VOLTAGE_1_00),
    MLUA_SYM_V(VOLTAGE_1_05, integer, VREG_VOLTAGE_1_05),
    MLUA_SYM_V(VOLTAGE_1_10, integer, VREG_VOLTAGE_1_10),
    MLUA_SYM_V(VOLTAGE_1_15, integer, VREG_VOLTAGE_1_15),
    MLUA_SYM_V(VOLTAGE_1_20, integer, VREG_VOLTAGE_1_20),
    MLUA_SYM_V(VOLTAGE_1_25, integer, VREG_VOLTAGE_1_25),
    MLUA_SYM_V(VOLTAGE_1_30, integer, VREG_VOLTAGE_1_30),
    MLUA_SYM_V(VOLTAGE_MIN, integer, VREG_VOLTAGE_MIN),
    MLUA_SYM_V(VOLTAGE_DEFAULT, integer, VREG_VOLTAGE_DEFAULT),
    MLUA_SYM_V(VOLTAGE_MAX, integer, VREG_VOLTAGE_MAX),

    MLUA_SYM_F(set_voltage, mod_),
};

MLUA_OPEN_MODULE(hardware.vreg) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
