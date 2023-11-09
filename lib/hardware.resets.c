// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/resets.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

MLUA_FUNC_V1(mod_,, reset_block, luaL_checkinteger)
MLUA_FUNC_V1(mod_,, unreset_block, luaL_checkinteger)
MLUA_FUNC_V1(mod_,, unreset_block_wait, luaL_checkinteger)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(reset_block, mod_),
    MLUA_SYM_F(unreset_block, mod_),
    MLUA_SYM_F(unreset_block_wait, mod_),
};

MLUA_OPEN_MODULE(hardware.resets) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
