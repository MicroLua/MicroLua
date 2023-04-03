#include "lua.h"
#include "lauxlib.h"

#include "mlua/util.h"

#include "hardware/gpio.h"

MLUA_FUNC_0_2(gpio_, set_function, integer, integer)
MLUA_FUNC_1_1(gpio_, get_function, integer, integer)
MLUA_FUNC_0_3(gpio_, set_pulls, integer, cbool, cbool)
MLUA_FUNC_0_1(gpio_, pull_up, integer)
MLUA_FUNC_1_1(gpio_, is_pulled_up, boolean, integer)
MLUA_FUNC_0_1(gpio_, pull_down, integer)
MLUA_FUNC_1_1(gpio_, is_pulled_down, boolean, integer)
MLUA_FUNC_0_1(gpio_, disable_pulls, integer)
MLUA_FUNC_0_2(gpio_, set_irqover, integer, integer)
MLUA_FUNC_0_2(gpio_, set_outover, integer, integer)
MLUA_FUNC_0_2(gpio_, set_inover, integer, integer)
MLUA_FUNC_0_2(gpio_, set_oeover, integer, integer)
MLUA_FUNC_0_2(gpio_, set_input_enabled, integer, cbool)
MLUA_FUNC_0_2(gpio_, set_input_hysteresis_enabled, integer, cbool)
MLUA_FUNC_1_1(gpio_, is_input_hysteresis_enabled, boolean, integer)
MLUA_FUNC_0_2(gpio_, set_slew_rate, integer, integer)
MLUA_FUNC_1_1(gpio_, get_slew_rate, integer, integer)
MLUA_FUNC_0_2(gpio_, set_drive_strength, integer, integer)
MLUA_FUNC_1_1(gpio_, get_drive_strength, integer, integer)
MLUA_FUNC_0_1(gpio_, init, integer)
MLUA_FUNC_0_1(gpio_, deinit, integer)
MLUA_FUNC_0_1(gpio_, init_mask, integer)
MLUA_FUNC_1_1(gpio_, get, boolean, integer)
MLUA_FUNC_1_0(gpio_, get_all, integer)
MLUA_FUNC_0_1(gpio_, set_mask, integer)
MLUA_FUNC_0_1(gpio_, clr_mask, integer)
MLUA_FUNC_0_1(gpio_, xor_mask, integer)
MLUA_FUNC_0_2(gpio_, put_masked, integer, integer)
MLUA_FUNC_0_1(gpio_, put_all, integer)
MLUA_FUNC_0_2(gpio_, put, integer, cbool)
MLUA_FUNC_1_1(gpio_, get_out_level, boolean, integer)
MLUA_FUNC_0_1(gpio_, set_dir_out_masked, integer)
MLUA_FUNC_0_1(gpio_, set_dir_in_masked, integer)
MLUA_FUNC_0_2(gpio_, set_dir_masked, integer, integer)
MLUA_FUNC_0_1(gpio_, set_dir_all_bits, integer)
MLUA_FUNC_0_2(gpio_, set_dir, integer, cbool)
MLUA_FUNC_1_1(gpio_, is_dir_out, boolean, integer)
MLUA_FUNC_1_1(gpio_, get_dir, integer, integer)

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(function, n, l_ ## n)
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
