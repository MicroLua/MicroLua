// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/clocks.h"  // BUG(pico-sdk): Missing include in hardware/pll.h
#include "hardware/pll.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"
#include "mlua/util.h"

MLUA_FUNC_V5(mod_, pll_, init, mlua_check_userdata, luaL_checkinteger,
             luaL_checkinteger, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_V1(mod_, pll_, deinit, mlua_check_userdata)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(VCO_MIN_FREQ_HZ, integer, PICO_PLL_VCO_MIN_FREQ_HZ),
    MLUA_SYM_V(VCO_MAX_FREQ_HZ, integer, PICO_PLL_VCO_MAX_FREQ_HZ),
    MLUA_SYM_V(sys, lightuserdata, pll_sys),
    MLUA_SYM_V(usb, lightuserdata, pll_usb),

    MLUA_SYM_F(init, mod_),
    MLUA_SYM_F(deinit, mod_),
};

MLUA_OPEN_MODULE(hardware.pll) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
