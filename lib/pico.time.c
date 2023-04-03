#include "lua.h"
#include "lauxlib.h"

#include "pico/time.h"

static int l_sleep_ms(lua_State *ls) {
    sleep_ms(luaL_checkinteger(ls, 1));
    return 0;
}

static luaL_Reg const module_funcs[] = {
    {"sleep_ms", l_sleep_ms},
    {NULL, NULL},
};

int luaopen_pico_time(lua_State* ls) {
    luaL_newlib(ls, module_funcs);
    return 1;
}
