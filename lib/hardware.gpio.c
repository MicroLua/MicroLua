#include "hardware/gpio.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

MLUA_FUNC_0_2(mod_, gpio_, set_function, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, gpio_, get_function, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_0_3(mod_, gpio_, set_pulls, luaL_checkinteger, mlua_to_cbool,
              mlua_to_cbool)
MLUA_FUNC_0_1(mod_, gpio_, pull_up, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, gpio_, is_pulled_up, lua_pushboolean, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, pull_down, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, gpio_, is_pulled_down, lua_pushboolean, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, disable_pulls, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_irqover, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_outover, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_inover, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_oeover, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_input_enabled, luaL_checkinteger, mlua_to_cbool)
MLUA_FUNC_0_2(mod_, gpio_, set_input_hysteresis_enabled, luaL_checkinteger,
              mlua_to_cbool)
MLUA_FUNC_1_1(mod_, gpio_, is_input_hysteresis_enabled, lua_pushboolean,
              luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_slew_rate, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, gpio_, get_slew_rate, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_drive_strength, luaL_checkinteger,
              luaL_checkinteger)
MLUA_FUNC_1_1(mod_, gpio_, get_drive_strength, lua_pushinteger,
              luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, init, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, deinit, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, init_mask, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, gpio_, get, lua_pushboolean, luaL_checkinteger)
MLUA_FUNC_1_0(mod_, gpio_, get_all, lua_pushinteger)
MLUA_FUNC_0_1(mod_, gpio_, set_mask, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, clr_mask, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, xor_mask, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, put_masked, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, put_all, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, put, luaL_checkinteger, mlua_to_cbool)
MLUA_FUNC_1_1(mod_, gpio_, get_out_level, lua_pushboolean, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, set_dir_out_masked, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, set_dir_in_masked, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_dir_masked, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, set_dir_all_bits, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_dir, luaL_checkinteger, mlua_to_cbool)
MLUA_FUNC_1_1(mod_, gpio_, is_dir_out, lua_pushboolean, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, gpio_, get_dir, lua_pushinteger, luaL_checkinteger)

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(function, n, mod_ ## n)
    X(set_function),
    X(get_function),
    X(set_pulls),
    X(pull_up),
    X(is_pulled_up),
    X(pull_down),
    X(is_pulled_down),
    X(disable_pulls),
    X(set_irqover),
    X(set_outover),
    X(set_inover),
    X(set_oeover),
    X(set_input_enabled),
    X(set_input_hysteresis_enabled),
    X(is_input_hysteresis_enabled),
    X(set_slew_rate),
    X(get_slew_rate),
    X(set_drive_strength),
    X(get_drive_strength),
    // X(set_irq_enabled),
    // X(set_irq_callback),
    // X(set_irq_enabled_with_callback),
    // X(set_dormant_irq_enabled),
    // X(get_irq_event_mask),
    // X(acknowledge_irq),
    // X(add_raw_irq_handler_with_order_priority_masked),
    // X(add_raw_irq_handler_with_order_priority),
    // X(add_raw_irq_handler_masked),
    // X(add_raw_irq_handler),
    // X(remove_raw_irq_handler_masked),
    // X(remove_raw_irq_handler),
    X(init),
    X(deinit),
    X(init_mask),
    X(get),
    X(get_all),
    X(set_mask),
    X(clr_mask),
    X(xor_mask),
    X(put_masked),
    X(put_all),
    X(put),
    X(get_out_level),
    X(set_dir_out_masked),
    X(set_dir_in_masked),
    X(set_dir_masked),
    X(set_dir_all_bits),
    X(set_dir),
    X(is_dir_out),
    X(get_dir),
#undef X
#define X(n) MLUA_REG(integer, n, GPIO_ ## n)
    X(IN),
    X(OUT),
#undef X
#define X(n) MLUA_REG(integer, DRIVE_STRENGTH ## n, GPIO_DRIVE_STRENGTH ## n)
    X(_2MA),
    X(_4MA),
    X(_8MA),
    X(_12MA),
#undef X
#define X(n) MLUA_REG(integer, FUNC_ ## n, GPIO_FUNC_ ## n)
    X(XIP),
    X(SPI),
    X(UART),
    X(I2C),
    X(PWM),
    X(SIO),
    X(PIO0),
    X(PIO1),
    X(GPCK),
    X(USB),
    X(NULL),
#undef X
#define X(n) MLUA_REG(integer, IRQ_ ## n, GPIO_IRQ_ ## n)
    X(LEVEL_LOW),
    X(LEVEL_HIGH),
    X(EDGE_FALL),
    X(EDGE_RISE),
#undef X
#define X(n) MLUA_REG(integer, OVERRIDE_ ## n, GPIO_OVERRIDE_ ## n)
    X(NORMAL),
    X(INVERT),
    X(LOW),
    X(HIGH),
#undef X
#define X(n) MLUA_REG(integer, SLEW_RATE_ ## n, GPIO_SLEW_RATE_ ## n)
    X(SLOW),
    X(FAST),
#undef X
    {NULL},
};

int luaopen_hardware_gpio(lua_State* ls) {
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
