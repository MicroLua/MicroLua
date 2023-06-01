#include "pico/stdlib.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

static int mod_check_sys_clock_khz(lua_State* ls) {
    lua_Integer freq = luaL_checkinteger(ls, 1);
    uint vco_freq, post_div1, post_div2;
    if (!check_sys_clock_khz(freq, &vco_freq, &post_div1, &post_div2)) return 0;
    lua_pushinteger(ls, vco_freq);
    lua_pushinteger(ls, post_div1);
    lua_pushinteger(ls, post_div2);
    return 3;
}

MLUA_FUNC_0_0(mod_,, setup_default_uart)
MLUA_FUNC_0_0(mod_,, set_sys_clock_48mhz)
MLUA_FUNC_0_3(mod_,, set_sys_clock_pll, luaL_checkinteger, luaL_checkinteger,
              luaL_checkinteger)
MLUA_FUNC_0_2(mod_,, set_sys_clock_khz, luaL_checkinteger, mlua_to_cbool)

static mlua_reg const module_regs[] = {
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(setup_default_uart),
    MLUA_SYM(set_sys_clock_48mhz),
    MLUA_SYM(set_sys_clock_pll),
    MLUA_SYM(set_sys_clock_khz),
#undef MLUA_SYM
    {NULL},
};

int luaopen_pico_stdlib(lua_State* ls) {
    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
