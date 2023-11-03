// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "pico/stdio_semihosting.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

MLUA_FUNC_0_0(mod_, stdio_semihosting_, init)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(driver, lightuserdata, &stdio_semihosting),

    MLUA_SYM_F(init, mod_),
};

MLUA_OPEN_MODULE(pico.stdio.semihosting) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
