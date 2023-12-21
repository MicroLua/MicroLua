// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/fs.h"

#include "mlua/module.h"
#include "mlua/util.h"

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(TYPE_REG, integer, MLUA_FS_TYPE_REG),
    MLUA_SYM_V(TYPE_DIR, integer, MLUA_FS_TYPE_DIR),
    MLUA_SYM_V(O_RDONLY, integer, MLUA_FS_O_RDONLY),
    MLUA_SYM_V(O_WRONLY, integer, MLUA_FS_O_WRONLY),
    MLUA_SYM_V(O_RDWR, integer, MLUA_FS_O_RDWR),
    MLUA_SYM_V(O_CREAT, integer, MLUA_FS_O_CREAT),
    MLUA_SYM_V(O_EXCL, integer, MLUA_FS_O_EXCL),
    MLUA_SYM_V(O_TRUNC, integer, MLUA_FS_O_TRUNC),
    MLUA_SYM_V(O_APPEND, integer, MLUA_FS_O_APPEND),
    MLUA_SYM_V(SEEK_SET, integer, MLUA_FS_SEEK_SET),
    MLUA_SYM_V(SEEK_CUR, integer, MLUA_FS_SEEK_CUR),
    MLUA_SYM_V(SEEK_END, integer, MLUA_FS_SEEK_END),
};

MLUA_OPEN_MODULE(mlua.fs) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
