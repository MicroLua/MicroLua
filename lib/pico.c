#include "lua.h"
#include "lauxlib.h"

#include "mlua/util.h"

#include "pico.h"

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(integer, n, PICO_ ## n)
#ifdef PICO_DEFAULT_LED_PIN
    X(DEFAULT_LED_PIN),
#endif
#undef X
    {NULL},
};

int luaopen_pico(lua_State* ls) {
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
