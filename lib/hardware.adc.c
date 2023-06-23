#include "hardware/adc.h"
#include "hardware/irq.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/util.h"

#if LIB_MLUA_MOD_MLUA_EVENT

static MLuaEvent event = MLUA_EVENT_UNSET;

static void __time_critical_func(handle_irq)(void) {
    adc_irq_set_enabled(false);
    mlua_event_set(event);
}

static int mod_fifo_enable_irq(lua_State* ls) {
    char const* err = mlua_event_enable_irq(ls, &event, ADC_IRQ_FIFO,
                                            &handle_irq, 1, -1);
    if (err != NULL) return luaL_error(ls, "ADC: %s", err);
    return 0;
}

#endif  // LIB_MLUA_MOD_MLUA_EVENT

static int try_fifo_get(lua_State* ls) {
    if (!adc_fifo_is_empty()) {
        lua_pushinteger(ls, adc_fifo_get());
        return 1;
    }
    adc_irq_set_enabled(true);
    return -1;
}

static int mod_fifo_get_blocking(lua_State* ls) {
    if (mlua_yield_enabled()) {
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

static MLuaReg const module_regs[] = {
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(init),
    MLUA_SYM(gpio_init),
    MLUA_SYM(select_input),
    MLUA_SYM(get_selected_input),
    MLUA_SYM(set_round_robin),
    MLUA_SYM(set_temp_sensor_enabled),
    MLUA_SYM(read),
    MLUA_SYM(run),
    MLUA_SYM(set_clkdiv),
    MLUA_SYM(fifo_setup),
    MLUA_SYM(fifo_is_empty),
    MLUA_SYM(fifo_get_level),
    MLUA_SYM(fifo_get),
    MLUA_SYM(fifo_get_blocking),
    MLUA_SYM(fifo_drain),
#if LIB_MLUA_MOD_MLUA_EVENT
    MLUA_SYM(fifo_enable_irq),
#endif
#undef MLUA_SYM
};

int luaopen_hardware_adc(lua_State* ls) {
#if LIB_MLUA_MOD_MLUA_EVENT
    mlua_require(ls, "mlua.event", false);
#endif

    // Create the module.
    mlua_new_table(ls, module_regs);
    return 1;
}
