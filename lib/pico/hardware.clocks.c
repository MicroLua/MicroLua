// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/clocks.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

static int mod_check_sys_clock_khz(lua_State* ls) {
    lua_Integer freq = luaL_checkinteger(ls, 1);
    uint vco_freq, post_div1, post_div2;
    if (!check_sys_clock_khz(freq, &vco_freq, &post_div1, &post_div2)) return 0;
    lua_pushinteger(ls, vco_freq);
    lua_pushinteger(ls, post_div1);
    lua_pushinteger(ls, post_div2);
    return 3;
}

MLUA_FUNC_R5(mod_, clock_, configure, lua_pushboolean, luaL_checkinteger,
             luaL_checkinteger, luaL_checkinteger, luaL_checkinteger,
             luaL_checkinteger)
MLUA_FUNC_V1(mod_, clock_, stop, luaL_checkinteger)
MLUA_FUNC_R1(mod_, clock_, get_hz, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_R1(mod_,, frequency_count_khz, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_V2(mod_, clock_, set_reported_hz, luaL_checkinteger,
             luaL_checkinteger)
MLUA_FUNC_V4(mod_, clock_, gpio_init_int_frac, luaL_checkinteger,
             luaL_checkinteger, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_V3(mod_, clock_, gpio_init, luaL_checkinteger, luaL_checkinteger,
             luaL_checknumber)
MLUA_FUNC_R4(mod_, clock_, configure_gpin, lua_pushboolean, luaL_checkinteger,
             luaL_checkinteger, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_V0(mod_,, set_sys_clock_48mhz)
MLUA_FUNC_V3(mod_,, set_sys_clock_pll, luaL_checkinteger, luaL_checkinteger,
             luaL_checkinteger)
MLUA_FUNC_R2(mod_,, set_sys_clock_khz, lua_pushboolean, luaL_checkinteger,
             mlua_to_cbool)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(KHZ, integer, KHZ),
    MLUA_SYM_V(MHZ, integer, MHZ),
    MLUA_SYM_V(clk_gpout0, integer, clk_gpout0),
    MLUA_SYM_V(clk_gpout1, integer, clk_gpout1),
    MLUA_SYM_V(clk_gpout2, integer, clk_gpout2),
    MLUA_SYM_V(clk_gpout3, integer, clk_gpout3),
    MLUA_SYM_V(clk_ref, integer, clk_ref),
    MLUA_SYM_V(clk_sys, integer, clk_sys),
    MLUA_SYM_V(clk_peri, integer, clk_peri),
    MLUA_SYM_V(clk_usb, integer, clk_usb),
    MLUA_SYM_V(clk_adc, integer, clk_adc),
    MLUA_SYM_V(clk_rtc, integer, clk_rtc),
    MLUA_SYM_V(CLK_COUNT, integer, CLK_COUNT),

    // clocks_init: Not useful in Lua (called by runtime)
    MLUA_SYM_F(configure, mod_),
    MLUA_SYM_F(stop, mod_),
    MLUA_SYM_F(get_hz, mod_),
    MLUA_SYM_F(frequency_count_khz, mod_),
    MLUA_SYM_F(set_reported_hz, mod_),
    // TODO: MLUA_SYM_F(enable_resus, mod_),
    MLUA_SYM_F(gpio_init_int_frac, mod_),
    MLUA_SYM_F(gpio_init, mod_),
    MLUA_SYM_F(configure_gpin, mod_),
    MLUA_SYM_F(set_sys_clock_48mhz, mod_),
    MLUA_SYM_F(set_sys_clock_pll, mod_),
    MLUA_SYM_F(check_sys_clock_khz, mod_),
    MLUA_SYM_F(set_sys_clock_khz, mod_),
};

MLUA_OPEN_MODULE(hardware.clocks) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
