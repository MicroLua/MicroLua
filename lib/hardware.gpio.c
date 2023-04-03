#include "lua.h"
#include "lauxlib.h"

#include "mlua/util.h"

#include "hardware/gpio.h"

static bool check_cbool(lua_State* ls, int arg) {
    if (lua_isinteger(ls, arg)) return lua_tointeger(ls, arg);
    if (lua_isboolean(ls, arg)) return lua_toboolean(ls, arg);
    return luaL_argerror(ls, arg, "boolean or integer expected");
}

static int l_init(lua_State* ls) {
    gpio_init(luaL_checkinteger(ls, 1));
    return 0;
}

static int l_set_dir(lua_State* ls) {
    gpio_set_dir(luaL_checkinteger(ls, 1), check_cbool(ls, 2));
    return 0;
}

static int l_put(lua_State* ls) {
    gpio_put(luaL_checkinteger(ls, 1), check_cbool(ls, 2));
    return 0;
}

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(function, n, l_ ## n)
    X(init),
    X(put),
    X(set_dir),
#undef X
#define X(n) MLUA_REG(integer, n, GPIO_ ## n)
    X(IN),
    X(OUT),
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
#define X(n) MLUA_REG(integer, SLEW_RATE_ ## n, GPIO_SLEW_RATE_ ## n)
    X(SLOW),
    X(FAST),
#undef X
#define X(n) MLUA_REG(integer, DRIVE_STRENGTH ## n, GPIO_DRIVE_STRENGTH ## n)
    X(_2MA),
    X(_4MA),
    X(_8MA),
    X(_12MA),
#undef X
    {NULL},
};

int luaopen_hardware_gpio(lua_State* ls) {
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
