// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>

#include "cyw43.h"
#include "pico/cyw43_driver.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

static int mod_init(lua_State* ls) {
    async_context_t* ctx = mlua_async_context();
    bool res = cyw43_driver_init(ctx);
    if (!res) cyw43_driver_deinit(ctx);
    return lua_pushboolean(ls, res), 1;
}

static int mod_deinit(lua_State* ls) {
    cyw43_driver_deinit(mlua_async_context());
    return 0;
}

#if CYW43_GPIO

static int check_gpio(lua_State* ls, int arg) {
    lua_Unsigned num = luaL_checkinteger(ls, arg);
    luaL_argcheck(ls, num < CYW43_WL_GPIO_COUNT, arg, "invalid CYW43 GPIO");
    return num;
}

static int mod_gpio_set(lua_State* ls) {
    int res = cyw43_gpio_set(&cyw43_state, check_gpio(ls, 1),
                             mlua_to_cbool(ls, 2));
    if (res != 0) return luaL_pushfail(ls), 1;
    return lua_pushboolean(ls, true), 1;
}

static int mod_gpio_get(lua_State* ls) {
    bool value;
    int res = cyw43_gpio_get(&cyw43_state, check_gpio(ls, 1), &value);
    if (res != 0) return luaL_pushfail(ls), 1;
    return lua_pushboolean(ls, value), 1;
}

#endif  // CYW43_GPIO

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(init, mod_),
    MLUA_SYM_F(deinit, mod_),
#if CYW43_GPIO
    MLUA_SYM_F(gpio_set, mod_),
    MLUA_SYM_F(gpio_get, mod_),
#else
    MLUA_SYM_V(gpio_set, boolean, false),
    MLUA_SYM_V(gpio_get, boolean, false),
#endif
};

MLUA_OPEN_MODULE(pico.cyw43) {
    mlua_thread_require(ls);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
