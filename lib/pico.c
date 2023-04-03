#include "lua.h"
#include "lauxlib.h"

#include "pico.h"

static luaL_Reg const module_funcs[] = {
    {"DEFAULT_LED_PIN", NULL},
    {NULL, NULL},
};

int luaopen_pico(lua_State* ls) {
    luaL_newlib(ls, module_funcs);
    lua_pushinteger(ls, PICO_DEFAULT_LED_PIN);
    lua_setfield(ls, -2, "DEFAULT_LED_PIN");
    return 1;
}
