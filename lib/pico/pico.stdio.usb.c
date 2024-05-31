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

// When true, initialize USB-based stdio.
#ifndef MLUA_STDIO_INIT_USB
#define MLUA_STDIO_INIT_USB 0
#endif

static __attribute__((constructor)) void init(void) {
#if MLUA_STDIO_INIT_USB
    stdio_usb_init();
#endif
}

MLUA_OPEN_MODULE(pico.stdio.usb) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
