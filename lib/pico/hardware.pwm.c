// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <assert.h>

#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico.h"

#include "mlua/hardware.gpio.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

static uint check_slice(lua_State* ls, int arg) {
    lua_Unsigned num = luaL_checkinteger(ls, arg);
    luaL_argcheck(ls, num < NUM_PWM_SLICES, arg, "invalid PWM slice");
    return num;
}

static char const Config_name[] = "hardware.pwm.Config";

static inline pwm_config* check_Config(lua_State* ls, int arg) {
    return luaL_checkudata(ls, arg, Config_name);
}

static int Config_csr(lua_State* ls) {
    pwm_config const* cfg = check_Config(ls, 1);
    return lua_pushinteger(ls, cfg->csr), 1;
}

static int Config_div(lua_State* ls) {
    pwm_config const* cfg = check_Config(ls, 1);
    return lua_pushinteger(ls, cfg->div), 1;
}

static int Config_top(lua_State* ls) {
    pwm_config const* cfg = check_Config(ls, 1);
    return lua_pushinteger(ls, cfg->top), 1;
}

MLUA_FUNC_S2(Config_, pwm_config_, set_phase_correct, check_Config,
             mlua_to_cbool)
MLUA_FUNC_S2(Config_, pwm_config_, set_clkdiv, check_Config, luaL_checknumber)
MLUA_FUNC_S(Config_, pwm_config_, set_clkdiv_int_frac, check_Config(ls, 1),
            luaL_checkinteger(ls, 2), luaL_optinteger(ls, 3, 0));
MLUA_FUNC_S2(Config_, pwm_config_, set_clkdiv_mode, check_Config,
             luaL_checkinteger)
MLUA_FUNC_S3(Config_, pwm_config_, set_output_polarity, check_Config,
             mlua_to_cbool, mlua_to_cbool)
MLUA_FUNC_S2(Config_, pwm_config_, set_wrap, check_Config, luaL_checkinteger)

#define Config_set_clkdiv_int Config_set_clkdiv_int_frac

MLUA_SYMBOLS(Config_syms) = {
    MLUA_SYM_F(csr, Config_),
    MLUA_SYM_F(div, Config_),
    MLUA_SYM_F(top, Config_),
    MLUA_SYM_F(set_phase_correct, Config_),
    MLUA_SYM_F(set_clkdiv, Config_),
    MLUA_SYM_F(set_clkdiv_int_frac, Config_),
    MLUA_SYM_F(set_clkdiv_int, Config_),
    MLUA_SYM_F(set_clkdiv_mode, Config_),
    MLUA_SYM_F(set_output_polarity, Config_),
    MLUA_SYM_F(set_wrap, Config_),
};

static int mod_get_default_config(lua_State* ls) {
    pwm_config* cfg = lua_newuserdatauv(ls, sizeof(pwm_config), 0);
    luaL_getmetatable(ls, Config_name);
    lua_setmetatable(ls, -2);
    *cfg = pwm_get_default_config();
    return 1;
}

static int mod_regs(lua_State* ls) {
    lua_pushlightuserdata(ls, lua_isnoneornil(ls, 1) ? (void*)pwm_hw
                              : (void*)&pwm_hw->slice[check_slice(ls, 1)]);
    return 1;
}

#if LIB_MLUA_MOD_MLUA_THREAD

typedef struct PWMState {
    MLuaEvent event;
    uint8_t pending;
} PWMState;

static PWMState pwm_state;

static_assert(NUM_PWM_SLICES <= 8 * sizeof(uint8_t),
              "pending bitmask too small");

static void __time_critical_func(handle_pwm_irq)(void) {
    uint32_t mask = pwm_get_irq_status_mask();
    pwm_hw->intr = mask;  // Clear IRQ sources
    uint32_t save = save_and_disable_interrupts();
    pwm_state.pending |= mask;
    restore_interrupts(save);
    mlua_event_set(&pwm_state.event);
}

static int handle_pwm_event(lua_State* ls) {
    uint32_t save = save_and_disable_interrupts();
    uint8_t pending = pwm_state.pending;
    pwm_state.pending = 0;
    restore_interrupts(save);
    if (pending == 0) return 0;

    // Call the handler
    lua_pushvalue(ls, lua_upvalueindex(1));  // handler
    lua_pushinteger(ls, pending);
    return mlua_callk(ls, 1, 0, mlua_cont_return, 0);
}

static int pwm_handler_done(lua_State* ls) {
    irq_set_enabled(PWM_IRQ_WRAP, false);
    irq_remove_handler(PWM_IRQ_WRAP, &handle_pwm_irq);
    mlua_event_disable(ls, &pwm_state.event);
    return 0;
}

static int mod_set_irq_handler(lua_State* ls) {
    if (lua_isnone(ls, 1)) return 0;  // No argument => no change
    if (lua_isnil(ls, 1)) {  // Nil callback, kill the handler thread
        mlua_event_stop_handler(ls, &pwm_state.event);
        return 0;
    }

    // Set the IRQ handler.
    lua_Integer priority = mlua_event_parse_irq_priority(ls, 2, -1);
    if (!mlua_event_enable(ls, &pwm_state.event)) {
        return luaL_error(ls, "PWM: handler already set");
    }
    mlua_event_set_irq_handler(PWM_IRQ_WRAP, &handle_pwm_irq, priority);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    // Start the event handler thread.
    lua_pushvalue(ls, 1);  // handler
    lua_pushcclosure(ls, &handle_pwm_event, 1);
    lua_pushcfunction(ls, &pwm_handler_done);
    return mlua_event_handle(ls, &pwm_state.event, &mlua_cont_return, 1);
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

static int mod_clear_irq(lua_State* ls) {
    uint slice = check_slice(ls, 1);
#if LIB_MLUA_MOD_MLUA_THREAD
    uint32_t save = save_and_disable_interrupts();
    pwm_clear_irq(slice);
    pwm_state.pending &= ~(1u << slice);
    restore_interrupts(save);
#else
    pwm_clear_irq(slice);
#endif
    return 0;
}

MLUA_FUNC_R1(mod_, pwm_, gpio_to_slice_num, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_R1(mod_, pwm_, gpio_to_channel, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_V3(mod_, pwm_, init, check_slice, check_Config, mlua_to_cbool)
MLUA_FUNC_V2(mod_, pwm_, set_wrap, check_slice, luaL_checkinteger)
MLUA_FUNC_V3(mod_, pwm_, set_chan_level, check_slice, luaL_checkinteger,
             luaL_checkinteger)
MLUA_FUNC_V3(mod_, pwm_, set_both_levels, check_slice, luaL_checkinteger,
             luaL_checkinteger)
MLUA_FUNC_V2(mod_, pwm_, set_gpio_level, mlua_check_gpio, luaL_checkinteger)
MLUA_FUNC_R1(mod_, pwm_, get_counter, lua_pushinteger, check_slice)
MLUA_FUNC_V2(mod_, pwm_, set_counter, check_slice, luaL_checkinteger)
MLUA_FUNC_V1(mod_, pwm_, advance_count, check_slice)
MLUA_FUNC_V1(mod_, pwm_, retard_count, check_slice)
MLUA_FUNC_V(mod_, pwm_, set_clkdiv_int_frac, check_slice(ls, 1),
            luaL_checkinteger(ls, 2), luaL_optinteger(ls, 3, 0));
MLUA_FUNC_V2(mod_, pwm_, set_clkdiv, check_slice, luaL_checknumber)
MLUA_FUNC_V3(mod_, pwm_, set_output_polarity, check_slice, mlua_to_cbool,
             mlua_to_cbool)
MLUA_FUNC_V2(mod_, pwm_, set_clkdiv_mode, check_slice, luaL_checkinteger)
MLUA_FUNC_V2(mod_, pwm_, set_phase_correct, check_slice, mlua_to_cbool)
MLUA_FUNC_V2(mod_, pwm_, set_enabled, check_slice, mlua_to_cbool)
MLUA_FUNC_V1(mod_, pwm_, set_mask_enabled, luaL_checkinteger)
MLUA_FUNC_V2(mod_, pwm_, set_irq_enabled, check_slice, mlua_to_cbool)
MLUA_FUNC_V2(mod_, pwm_, set_irq_mask_enabled, luaL_checkinteger, mlua_to_cbool)
MLUA_FUNC_R0(mod_, pwm_, get_irq_status_mask, lua_pushinteger)
MLUA_FUNC_V1(mod_, pwm_, force_irq, check_slice)
MLUA_FUNC_R1(mod_, pwm_, get_dreq, lua_pushinteger, check_slice)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(NUM_SLICES, integer, NUM_PWM_SLICES),
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
    MLUA_SYM_F(regs, mod_),
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
    MLUA_SYM_F_THREAD(set_irq_handler, mod_),
    MLUA_SYM_F(set_irq_enabled, mod_),
    MLUA_SYM_F(set_irq_mask_enabled, mod_),
    MLUA_SYM_F(clear_irq, mod_),
    MLUA_SYM_F(get_irq_status_mask, mod_),
    MLUA_SYM_F(force_irq, mod_),
    MLUA_SYM_F(get_dreq, mod_),
};

MLUA_OPEN_MODULE(hardware.pwm) {
    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the Config class.
    mlua_new_class(ls, Config_name, Config_syms, mlua_nosyms);
    lua_pop(ls, 1);
    return 1;
}
