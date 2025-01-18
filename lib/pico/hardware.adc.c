// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>

#include "hardware/adc.h"
#include "hardware/irq.h"
#include "pico.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

static uint check_channel(lua_State* ls, int arg) {
    lua_Unsigned num = luaL_checkinteger(ls, arg);
    luaL_argcheck(ls, num < NUM_ADC_CHANNELS, arg, "invalid ADC channel");
    return num;
}

#if LIB_MLUA_MOD_MLUA_THREAD

typedef struct ADCState {
    MLuaEvent event;
} ADCState;

static ADCState adc_state;

static void __time_critical_func(handle_adc_irq)(void) {
    adc_irq_set_enabled(false);
    mlua_event_set(&adc_state.event);
}

static int mod_fifo_enable_irq(lua_State* ls) {
    if (!mlua_event_enable_irq(ls, &adc_state.event, ADC_IRQ_FIFO,
                               &handle_adc_irq, 1, -1)) {
        return luaL_error(ls, "ADC: FIFO IRQ already enabled");
    }
    return 0;
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

static int fifo_get_loop(lua_State* ls, bool timeout) {
    if (adc_fifo_is_empty()) {
        adc_irq_set_enabled(true);
        return -1;
    }
    lua_pushinteger(ls, adc_fifo_get());
    return 1;
}

static int mod_fifo_get_blocking(lua_State* ls) {
#if LIB_MLUA_MOD_MLUA_THREAD
    if (mlua_event_can_wait(ls, &adc_state.event, 0)) {
        return mlua_event_wait(ls, &adc_state.event, 0, &fifo_get_loop, 0);
    }
#endif
    lua_pushinteger(ls, adc_fifo_get_blocking());
    return 1;
}

MLUA_FUNC_V0(mod_, adc_, init)
MLUA_FUNC_V1(mod_, adc_, gpio_init, luaL_checkinteger)
MLUA_FUNC_V1(mod_, adc_, select_input, check_channel)
MLUA_FUNC_R0(mod_, adc_, get_selected_input, lua_pushinteger)
MLUA_FUNC_V1(mod_, adc_, set_round_robin, luaL_checkinteger)
MLUA_FUNC_V1(mod_, adc_, set_temp_sensor_enabled, mlua_to_cbool)
MLUA_FUNC_R0(mod_, adc_, read, lua_pushinteger)
MLUA_FUNC_V1(mod_, adc_, run, mlua_to_cbool)
MLUA_FUNC_V1(mod_, adc_, set_clkdiv, luaL_checknumber)
MLUA_FUNC_V5(mod_, adc_, fifo_setup, mlua_to_cbool, mlua_to_cbool,
             luaL_checkinteger, mlua_to_cbool, mlua_to_cbool)
MLUA_FUNC_R0(mod_, adc_, fifo_is_empty, lua_pushboolean)
MLUA_FUNC_R0(mod_, adc_, fifo_get_level, lua_pushinteger)
MLUA_FUNC_R0(mod_, adc_, fifo_get, lua_pushinteger)
MLUA_FUNC_V0(mod_, adc_, fifo_drain)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(init, mod_),
    MLUA_SYM_F(gpio_init, mod_),
    MLUA_SYM_F(select_input, mod_),
    MLUA_SYM_F(get_selected_input, mod_),
    MLUA_SYM_F(set_round_robin, mod_),
    MLUA_SYM_F(set_temp_sensor_enabled, mod_),
    MLUA_SYM_F(read, mod_),
    MLUA_SYM_F(run, mod_),
    MLUA_SYM_F(set_clkdiv, mod_),
    MLUA_SYM_F(fifo_setup, mod_),
    MLUA_SYM_F(fifo_is_empty, mod_),
    MLUA_SYM_F(fifo_get_level, mod_),
    MLUA_SYM_F(fifo_get, mod_),
    MLUA_SYM_F(fifo_get_blocking, mod_),
    MLUA_SYM_F(fifo_drain, mod_),
    MLUA_SYM_F_THREAD(fifo_enable_irq, mod_),
};

MLUA_OPEN_MODULE(hardware.adc) {
    mlua_thread_require(ls);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
