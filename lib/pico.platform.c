#include "lua.h"
#include "lauxlib.h"

#include "mlua/util.h"

#include "pico/platform.h"

MLUA_FUNC_1_0(mod_,, rp2040_chip_version, lua_pushinteger)
MLUA_FUNC_1_0(mod_,, rp2040_rom_version, lua_pushinteger)
MLUA_FUNC_0_1(mod_,, busy_wait_at_least_cycles, luaL_checkinteger)
MLUA_FUNC_1_0(mod_,, get_core_num, lua_pushinteger)

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(function, n, mod_ ## n)
    X(rp2040_chip_version),
    X(rp2040_rom_version),
    X(busy_wait_at_least_cycles),
    X(get_core_num),
#undef X
#define X(n) MLUA_REG(integer, n, PICO_ ## n)
#ifdef PICO_RP2040
    X(RP2040),
#endif
#undef X
    {NULL},
};

int luaopen_pico_platform(lua_State* ls) {
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
