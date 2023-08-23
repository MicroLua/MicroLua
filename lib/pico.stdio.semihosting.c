#include "pico/stdio_semihosting.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

MLUA_FUNC_0_0(mod_, stdio_semihosting_, init)

static MLuaSym const module_syms[] = {
    MLUA_SYM_V(driver, lightuserdata, &stdio_semihosting),

    MLUA_SYM_F(init, mod_),
};

int luaopen_pico_stdio_semihosting(lua_State* ls) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}