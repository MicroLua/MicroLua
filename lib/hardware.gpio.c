#include "lua.h"
#include "lauxlib.h"

#include "hardware/gpio.h"

static bool check_cbool(lua_State* ls, int arg) {
    if (lua_isinteger(ls, arg)) return lua_tointeger(ls, arg);
    if (lua_isboolean(ls, arg)) return lua_toboolean(ls, arg);
    return luaL_argerror(ls, arg, "boolean or integer expected");
}

static int l_init(lua_State* ls) {
    gpio_init(luaL_checkinteger(ls, 1));
    return 0;
}

static int l_set_dir(lua_State* ls) {
    gpio_set_dir(luaL_checkinteger(ls, 1), check_cbool(ls, 2));
    return 0;
}

static int l_put(lua_State* ls) {
    gpio_put(luaL_checkinteger(ls, 1), check_cbool(ls, 2));
    return 0;
}

static luaL_Reg const module_funcs[] = {
    {"init", l_init},
    {"put", l_put},
    {"set_dir", l_set_dir},
    {"IN", NULL},
    {"OUT", NULL},
    {NULL, NULL},
};

#define mlua_set_field(type, name, value)   \
    do {                                    \
        lua_push ## type(ls, value);        \
        lua_setfield(ls, -2, #name);        \
    } while(0);

int luaopen_hardware_gpio(lua_State* ls) {
    luaL_newlib(ls, module_funcs);
    mlua_set_field(integer, IN, GPIO_IN);
    mlua_set_field(integer, OUT, GPIO_OUT);
    return 1;
}
