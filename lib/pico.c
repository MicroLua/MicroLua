#include "pico.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(integer, n, PICO_ ## n)
    X(OK),
    X(ERROR_NONE),
    X(ERROR_TIMEOUT),
    X(ERROR_GENERIC),
    X(ERROR_NO_DATA),
    X(ERROR_NOT_PERMITTED),
    X(ERROR_INVALID_ARG),
    X(ERROR_IO),
    X(ERROR_BADAUTH),
    X(ERROR_CONNECT_FAILED),
    X(SDK_VERSION_MAJOR),
    X(SDK_VERSION_MINOR),
    X(SDK_VERSION_REVISION),
#ifdef PICO_DEFAULT_LED_PIN
    X(DEFAULT_LED_PIN),
#endif
#undef X
#define X(n) MLUA_REG(string, n, PICO_ ## n)
    X(SDK_VERSION_STRING),
#undef X
    {NULL},
};

int luaopen_pico(lua_State* ls) {
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
