#include "pico/stdio_usb.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

MLUA_FUNC_1_0(mod_, stdio_usb_, init, lua_pushboolean)
MLUA_FUNC_1_0(mod_, stdio_usb_, connected, lua_pushboolean)

static MLuaSym const module_syms[] = {
    MLUA_SYM_V(driver, lightuserdata, &stdio_usb),

    MLUA_SYM_F(init, mod_),
    MLUA_SYM_F(connected, mod_),
};

int luaopen_pico_stdio_usb(lua_State* ls) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
