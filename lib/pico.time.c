#include "lua.h"
#include "lauxlib.h"

#include "mlua/util.h"

#include "pico/time.h"

MLUA_FUNC_0_1(, sleep_ms, integer)

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
