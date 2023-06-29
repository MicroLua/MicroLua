#include "pico/stdlib.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

static int mod_check_sys_clock_khz(lua_State* ls) {
    lua_Integer freq = luaL_checkinteger(ls, 1);
    uint vco_freq, post_div1, post_div2;
    if (!check_sys_clock_khz(freq, &vco_freq, &post_div1, &post_div2)) {
        lua_pushboolean(ls, false);
        return 1;
    }
    lua_pushinteger(ls, vco_freq);
    lua_pushinteger(ls, post_div1);
    lua_pushinteger(ls, post_div2);
    return 3;
}

MLUA_FUNC_0_0(mod_,, setup_default_uart)
MLUA_FUNC_0_0(mod_,, set_sys_clock_48mhz)
MLUA_FUNC_0_3(mod_,, set_sys_clock_pll, luaL_checkinteger, luaL_checkinteger,
              luaL_checkinteger)
MLUA_FUNC_1_2(mod_,, set_sys_clock_khz, lua_pushboolean, luaL_checkinteger,
              mlua_to_cbool)

static MLuaSym const module_syms[] = {
    MLUA_SYM_F(setup_default_uart, mod_),
    MLUA_SYM_F(set_sys_clock_48mhz, mod_),
    MLUA_SYM_F(set_sys_clock_pll, mod_),
    MLUA_SYM_F(check_sys_clock_khz, mod_),
    MLUA_SYM_F(set_sys_clock_khz, mod_),
};

int luaopen_pico_stdlib(lua_State* ls) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
