#include "pico/stdio_uart.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

MLUA_FUNC_0_0(mod_, stdio_uart_, init)

static MLuaSym const module_syms[] = {
    MLUA_SYM_V(driver, lightuserdata, &stdio_uart),

    MLUA_SYM_F(init, mod_),
    // TODO: MLUA_SYM_F(init_stdout, mod_),
    // TODO: MLUA_SYM_F(init_stdin, mod_),
    // TODO: MLUA_SYM_F(init_full, mod_),
};

int luaopen_pico_stdio_uart(lua_State* ls) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
