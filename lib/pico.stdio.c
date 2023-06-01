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
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(init_all),
    MLUA_SYM(flush),
    MLUA_SYM(getchar_timeout_us),
    // MLUA_SYM(set_driver_enabled),
    // MLUA_SYM(filter_driver),
    // MLUA_SYM(set_translate_crlf),
    MLUA_SYM(putchar_raw),
    MLUA_SYM(puts_raw),
    // MLUA_SYM(set_chars_available_callback),
#undef MLUA_SYM
    {NULL},
};

int luaopen_pico_stdio(lua_State* ls) {
    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
