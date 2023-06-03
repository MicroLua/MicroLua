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
    MLUA_SYM(irq_set_enabled),
    MLUA_SYM(irq_set_handler),
#undef MLUA_SYM
    {NULL},
};

int luaopen_hardware_adc(lua_State* ls) {
    mlua_require(ls, "hardware.irq", false);

    // Initialize internal state.
    uint32_t save = save_and_disable_interrupts();
    enabled[get_core_num()] = false;
    restore_interrupts(save);

    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
