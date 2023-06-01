#include "pico/stdio.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

MLUA_FUNC_1_0(mod_, stdio_, init_all, lua_pushboolean)
MLUA_FUNC_0_0(mod_, stdio_, flush)
MLUA_FUNC_1_1(mod_,, getchar_timeout_us, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_1_1(mod_,, putchar_raw, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_1_1(mod_,, puts_raw, lua_pushinteger, luaL_checkstring)

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(function, n, mod_ ## n)
    X(init_all),
    X(flush),
    X(getchar_timeout_us),
    // X(set_driver_enabled),
    // X(filter_driver),
    // X(set_translate_crlf),
    X(putchar_raw),
    X(puts_raw),
    // X(set_chars_available_callback),
#undef X
    {NULL},
};

int luaopen_pico_stdio(lua_State* ls) {
    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
