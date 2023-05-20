#include "hardware/adc.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/hardware.irq.h"
#include "mlua/util.h"

static bool enabled[NUM_CORES];

static void __time_critical_func(handle_irq)(void) {
    adc_irq_set_enabled(false);  // ADC IRQ is level-triggered
    mlua_irq_set_signal(ADC_IRQ_FIFO, true);
}

static int handle_signal(lua_State* ls) {
    lua_pushvalue(ls, lua_upvalueindex(1));
    lua_call(ls, 0, 0);
    adc_irq_set_enabled(enabled[get_core_num()]);
    return 0;
}

static int mod_irq_set_handler(lua_State* ls) {
    mlua_irq_set_handler(ls, ADC_IRQ_FIFO, &handle_irq, &handle_signal, 1);
    return 0;
}

static int mod_irq_set_enabled(lua_State* ls) {
    bool enable = mlua_to_cbool(ls, 1);
    enabled[get_core_num()] = enable;
    adc_irq_set_enabled(enable);
    return 0;
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
MLUA_FUNC_1_0(mod_, adc_, fifo_get_blocking, lua_pushinteger)
MLUA_FUNC_0_0(mod_, adc_, fifo_drain)

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(function, n, mod_ ## n)
    X(init),
    X(gpio_init),
    X(select_input),
    X(get_selected_input),
    X(set_round_robin),
    X(set_temp_sensor_enabled),
    X(read),
    X(run),
    X(set_clkdiv),
    X(fifo_setup),
    X(fifo_is_empty),
    X(fifo_get_level),
    X(fifo_get),
    X(fifo_get_blocking),
    X(fifo_drain),
    X(irq_set_enabled),
    X(irq_set_handler),
#undef X
    {NULL},
};

int luaopen_hardware_adc(lua_State* ls) {
    mlua_require(ls, "hardware.irq", false);

    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
