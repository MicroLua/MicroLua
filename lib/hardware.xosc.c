// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/xosc.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

MLUA_FUNC_V0(mod_, xosc_, init)
MLUA_FUNC_V0(mod_, xosc_, disable)
MLUA_FUNC_V0(mod_, xosc_, dormant)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(init, mod_),
    MLUA_SYM_F(disable, mod_),
    MLUA_SYM_F(dormant, mod_),
};

MLUA_OPEN_MODULE(hardware.xosc) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
