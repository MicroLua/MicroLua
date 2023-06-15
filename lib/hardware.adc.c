#include "hardware/adc.h"
#include "hardware/irq.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/util.h"

static MLuaEvent events[NUM_CORES];

static void __time_critical_func(handle_irq)(void) {
    adc_irq_set_enabled(false);
    mlua_event_set(events[get_core_num()], true);
}

static int mod_fifo_enable_irq(lua_State* ls) {
    mlua_event_enable_irq(ls, &events[get_core_num()], ADC_IRQ_FIFO,
                          &handle_irq, 1, -1);
    return 0;
}

static int mod_fifo_get_blocking_1(lua_State* ls);
static int mod_fifo_get_blocking_2(lua_State* ls, int status, lua_KContext ctx);

static int mod_fifo_get_blocking(lua_State* ls) {
    if (!adc_fifo_is_empty()) {
        lua_pushinteger(ls, adc_fifo_get());
        return 1;
    }
#if MLUA_BLOCK_WHEN_NON_YIELDABLE
    if (!lua_isyieldable(ls)) {
        lua_pushinteger(ls, adc_fifo_get_blocking());
        return 1;
    }
#endif
    mlua_event_watch(ls, events[get_core_num()]);
    return mod_fifo_get_blocking_1(ls);
}

static int mod_fifo_get_blocking_1(lua_State* ls) {
    adc_irq_set_enabled(true);
    return mlua_event_suspend(ls, &mod_fifo_get_blocking_2);
}

static int mod_fifo_get_blocking_2(lua_State* ls, int status,
                                   lua_KContext ctx) {
    if (adc_fifo_is_empty()) return mod_fifo_get_blocking_1(ls);
    mlua_event_unwatch(ls, events[get_core_num()]);
    lua_pushinteger(ls, adc_fifo_get());
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
    MLUA_SYM(fifo_enable_irq),
#undef MLUA_SYM
    {NULL},
};

int luaopen_hardware_adc(lua_State* ls) {
    mlua_require(ls, "mlua.event", false);

    // Initialize internal state.
    uint32_t save = save_and_disable_interrupts();
    events[get_core_num()] = -1;
    restore_interrupts(save);

    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
