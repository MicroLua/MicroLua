// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/module.h"

#include "@INCLUDE@"

#include "mlua/util.h"

MLUA_SYMBOLS_HASH(module_syms) = {
@SYMBOLS@
};

MLUA_OPEN_MODULE(@MOD@) {
    mlua_new_module_hash(ls, 0, module_syms);
    return 1;
}
