// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/pwm.h"
#include "pico/platform.h"

#include "mlua/module.h"
#include "mlua/util.h"

char const Config_name[] = "hardware.pwm.Config";

static uint check_slice(lua_State* ls, int arg) {
    uint num = luaL_checkinteger(ls, arg);
    luaL_argcheck(ls, num < NUM_PWM_SLICES, arg, "invalid PWM slice number");
    return num;
}

static inline pwm_config* check_Config(lua_State* ls, int arg) {
    return (pwm_config*)luaL_checkudata(ls, arg, Config_name);
}

static int mod_get_default_config(lua_State* ls) {
    pwm_config* cfg = lua_newuserdatauv(ls, sizeof(pwm_config), 0);
    luaL_getmetatable(ls, Config_name);
    lua_setmetatable(ls, -2);
    *cfg = pwm_get_default_config();
    return 1;
}

static int mod_reg_base(lua_State* ls) {
    lua_pushinteger(ls, lua_isnoneornil(ls, 1) ? (uintptr_t)pwm_hw
                        : (uintptr_t)&pwm_hw->slice[check_slice(ls, 1)]);
    return 1;
}

MLUA_FUNC_0_2(Config_, pwm_config_, set_phase_correct, check_Config,
              mlua_to_cbool)
MLUA_FUNC_0_2(Config_, pwm_config_, set_clkdiv, check_Config, luaL_checknumber)
MLUA_FUNC_V(Config_, pwm_config_, set_clkdiv_int_frac, check_Config(ls, 1),
            luaL_checkinteger(ls, 2), luaL_optinteger(ls, 3, 0));
MLUA_FUNC_0_2(Config_, pwm_config_, set_clkdiv_mode, check_Config,
              luaL_checkinteger)
MLUA_FUNC_0_3(Config_, pwm_config_, set_output_polarity, check_Config,
              mlua_to_cbool, mlua_to_cbool)
MLUA_FUNC_0_2(Config_, pwm_config_, set_wrap, check_Config, luaL_checkinteger)

MLUA_FUNC_1_1(mod_, pwm_, gpio_to_slice_num, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, pwm_, gpio_to_channel, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_0_3(mod_, pwm_, init, check_slice, check_Config, mlua_to_cbool)
MLUA_FUNC_0_2(mod_, pwm_, set_wrap, check_slice, luaL_checkinteger)
MLUA_FUNC_0_3(mod_, pwm_, set_chan_level, check_slice, luaL_checkinteger,
              luaL_checkinteger)
MLUA_FUNC_0_3(mod_, pwm_, set_both_levels, check_slice, luaL_checkinteger,
              luaL_checkinteger)
MLUA_FUNC_0_2(mod_, pwm_, set_gpio_level, mlua_check_gpio, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, pwm_, get_counter, lua_pushinteger, check_slice)
MLUA_FUNC_0_2(mod_, pwm_, set_counter, check_slice, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, pwm_, advance_count, check_slice)
MLUA_FUNC_0_1(mod_, pwm_, retard_count, check_slice)
MLUA_FUNC_V(mod_, pwm_, set_clkdiv_int_frac, check_slice(ls, 1),
            luaL_checkinteger(ls, 2), luaL_optinteger(ls, 3, 0));
MLUA_FUNC_0_2(mod_, pwm_, set_clkdiv, check_slice, luaL_checknumber)
MLUA_FUNC_0_3(mod_, pwm_, set_output_polarity, check_slice, mlua_to_cbool,
              mlua_to_cbool)
MLUA_FUNC_0_2(mod_, pwm_, set_clkdiv_mode, check_slice, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, pwm_, set_phase_correct, check_slice, mlua_to_cbool)
MLUA_FUNC_0_2(mod_, pwm_, set_enabled, check_slice, mlua_to_cbool)
MLUA_FUNC_0_1(mod_, pwm_, set_mask_enabled, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, pwm_, clear_irq, check_slice)
MLUA_FUNC_1_0(mod_, pwm_, get_irq_status_mask, lua_pushinteger)
MLUA_FUNC_0_1(mod_, pwm_, force_irq, check_slice)
MLUA_FUNC_1_1(mod_, pwm_, get_dreq, lua_pushinteger, check_slice)

#define Config_set_clkdiv_int Config_set_clkdiv_int_frac

MLUA_SYMBOLS(Config_syms) = {
    MLUA_SYM_F(set_phase_correct, Config_),
    MLUA_SYM_F(set_clkdiv, Config_),
    MLUA_SYM_F(set_clkdiv_int_frac, Config_),
    MLUA_SYM_F(set_clkdiv_int, Config_),
    MLUA_SYM_F(set_clkdiv_mode, Config_),
    MLUA_SYM_F(set_output_polarity, Config_),
    MLUA_SYM_F(set_wrap, Config_),
};

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(DIV_FREE_RUNNING, integer, PWM_DIV_FREE_RUNNING),
    MLUA_SYM_V(DIV_B_HIGH, integer, PWM_DIV_B_HIGH),
    MLUA_SYM_V(DIV_B_RISING, integer, PWM_DIV_B_RISING),
    MLUA_SYM_V(DIV_B_FALLING, integer, PWM_DIV_B_FALLING),
    MLUA_SYM_V(CHAN_A, integer, PWM_CHAN_A),
    MLUA_SYM_V(CHAN_B, integer, PWM_CHAN_B),

    MLUA_SYM_F(gpio_to_slice_num, mod_),
    MLUA_SYM_F(gpio_to_channel, mod_),
    MLUA_SYM_F(init, mod_),
    MLUA_SYM_F(get_default_config, mod_),
    MLUA_SYM_F(reg_base, mod_),
    MLUA_SYM_F(set_wrap, mod_),
    MLUA_SYM_F(set_chan_level, mod_),
    MLUA_SYM_F(set_both_levels, mod_),
    MLUA_SYM_F(set_gpio_level, mod_),
    MLUA_SYM_F(get_counter, mod_),
    MLUA_SYM_F(set_counter, mod_),
    MLUA_SYM_F(advance_count, mod_),
    MLUA_SYM_F(retard_count, mod_),
    MLUA_SYM_F(set_clkdiv_int_frac, mod_),
    MLUA_SYM_F(set_clkdiv, mod_),
    MLUA_SYM_F(set_output_polarity, mod_),
    MLUA_SYM_F(set_clkdiv_mode, mod_),
    MLUA_SYM_F(set_phase_correct, mod_),
    MLUA_SYM_F(set_enabled, mod_),
    MLUA_SYM_F(set_mask_enabled, mod_),
    // TODO: MLUA_SYM_F(set_irq_enabled, mod_),
    // TODO: MLUA_SYM_F(set_irq_mask_enabled, mod_),
    MLUA_SYM_F(clear_irq, mod_),
    MLUA_SYM_F(get_irq_status_mask, mod_),
    MLUA_SYM_F(force_irq, mod_),
    MLUA_SYM_F(get_dreq, mod_),
};

MLUA_OPEN_MODULE(hardware.pwm) {
    mlua_new_class(ls, Config_name, Config_syms, true);
    lua_pop(ls, 1);
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
