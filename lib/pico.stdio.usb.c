// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "pico/stdio_usb.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

MLUA_FUNC_R0(mod_, stdio_usb_, init, lua_pushboolean)
MLUA_FUNC_R0(mod_, stdio_usb_, connected, lua_pushboolean)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(driver, lightuserdata, &stdio_usb),

    MLUA_SYM_F(init, mod_),
    MLUA_SYM_F(connected, mod_),
};

MLUA_OPEN_MODULE(pico.stdio.usb) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
