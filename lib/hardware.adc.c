#include "lua.h"
#include "lauxlib.h"

#include "mlua/util.h"

#include "hardware/adc.h"

MLUA_FUNC_0_0(adc_, init)
MLUA_FUNC_0_1(adc_, gpio_init, integer)
MLUA_FUNC_0_1(adc_, select_input, integer)
MLUA_FUNC_1_0(adc_, get_selected_input, integer)
MLUA_FUNC_0_1(adc_, set_round_robin, integer)
MLUA_FUNC_0_1(adc_, set_temp_sensor_enabled, cbool)
MLUA_FUNC_1_0(adc_, read, integer)
MLUA_FUNC_0_1(adc_, run, cbool)
MLUA_FUNC_0_1(adc_, set_clkdiv, number)
MLUA_FUNC_0_5(adc_, fifo_setup, cbool, cbool, integer, cbool, cbool)
MLUA_FUNC_1_0(adc_, fifo_is_empty, boolean)
MLUA_FUNC_1_0(adc_, fifo_get_level, integer)
MLUA_FUNC_1_0(adc_, fifo_get, integer)
MLUA_FUNC_1_0(adc_, fifo_get_blocking, integer)
MLUA_FUNC_0_0(adc_, fifo_drain)
MLUA_FUNC_0_1(adc_, irq_set_enabled, cbool)

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(function, n, l_ ## n)
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
#undef X
    {NULL},
};

int luaopen_hardware_adc(lua_State* ls) {
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
