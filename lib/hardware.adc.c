#include <stdbool.h>

#include "hardware/adc.h"
#include "hardware/irq.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/util.h"

#if LIB_MLUA_MOD_MLUA_EVENT

static MLuaEvent event = MLUA_EVENT_UNSET;

static void __time_critical_func(handle_adc_irq)(void) {
    adc_irq_set_enabled(false);
    mlua_event_set(event);
}

static int mod_fifo_enable_irq(lua_State* ls) {
    char const* err = mlua_event_enable_irq(ls, &event, ADC_IRQ_FIFO,
                                            &handle_adc_irq, 1, -1);
    if (err != NULL) return luaL_error(ls, "ADC: %s", err);
    return 0;
}

#endif  // LIB_MLUA_MOD_MLUA_EVENT

static int try_fifo_get(lua_State* ls, bool timeout) {
    if (adc_fifo_is_empty()) {
        adc_irq_set_enabled(true);
        return -1;
    }
    lua_pushinteger(ls, adc_fifo_get());
    return 1;
}

static int mod_fifo_get_blocking(lua_State* ls) {
    if (mlua_event_can_wait(&event)) {
        return mlua_event_wait(ls, event, &try_fifo_get, 0);
    }
    lua_pushinteger(ls, adc_fifo_get_blocking());
    return 1;
}

MLUA_FUNC_0_0(mod_, adc_, init)
MLUA_FUNC_0_1(mod_, adc_, gpio_init, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, adc_, select_input, luaL_checkinteger)
MLUA_FUNC_1_0(mod_, adc_, get_selected_input, lua_pushinteger)
MLUA_FUNC_0_1(mod_, adc_, set_round_robin, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, adc_, set_temp_sensor_enabled, mlua_to_cbool)
MLUA_FUNC_1_0(mod_, adc_, read, lua_pushinteger)
MLUA_FUNC_0_1(mod_, adc_, run, mlua_to_cbool)
MLUA_FUNC_0_1(mod_, adc_, set_clkdiv, luaL_checknumber)
MLUA_FUNC_0_5(mod_, adc_, fifo_setup, mlua_to_cbool, mlua_to_cbool,
              luaL_checkinteger, mlua_to_cbool, mlua_to_cbool)
MLUA_FUNC_1_0(mod_, adc_, fifo_is_empty, lua_pushboolean)
MLUA_FUNC_1_0(mod_, adc_, fifo_get_level, lua_pushinteger)
MLUA_FUNC_1_0(mod_, adc_, fifo_get, lua_pushinteger)
MLUA_FUNC_0_0(mod_, adc_, fifo_drain)

static MLuaSym const module_syms[] = {
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
#if LIB_MLUA_MOD_MLUA_EVENT
    MLUA_SYM_F(fifo_enable_irq, mod_),
#endif
};

int luaopen_hardware_adc(lua_State* ls) {
    mlua_event_require(ls);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
