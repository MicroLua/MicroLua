#include "lua.h"
#include "lauxlib.h"

#include "mlua/util.h"

#include "pico/time.h"

static int l_sleep_ms(lua_State *ls) {
    sleep_ms(luaL_checkinteger(ls, 1));
    return 0;
}

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(function, n, l_ ## n)
    X(sleep_ms),
#undef X
    {NULL},
};

int luaopen_pico_time(lua_State* ls) {
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
