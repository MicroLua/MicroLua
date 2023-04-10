#include "lua.h"
#include "lauxlib.h"

#include "mlua/util.h"

#include "hardware/adc.h"

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
MLUA_FUNC_0_1(mod_, adc_, irq_set_enabled, mlua_to_cbool)

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
#undef X
    {NULL},
};

int luaopen_hardware_adc(lua_State* ls) {
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
