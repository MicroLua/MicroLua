// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/platform.h"

#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/util.h"

static void mod_flash(lua_State* ls, MLuaSymVal const* value) {
    MLuaFlash const* flash = mlua_platform_flash();
    if (flash == NULL)  {
        lua_pushboolean(ls, false);
        return;
    }
    lua_createtable(ls, 0, 4);
    lua_pushlightuserdata(ls, (void*)flash->ptr);
    lua_setfield(ls, -2, "ptr");
    mlua_push_size(ls, flash->size);
    lua_setfield(ls, -2, "size");
    mlua_push_size(ls, flash->write_size);
    lua_setfield(ls, -2, "write_size");
    mlua_push_size(ls, flash->erase_size);
    lua_setfield(ls, -2, "erase_size");
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(name, string, MLUA_ESTR(MLUA_PLATFORM)),
    MLUA_SYM_P(flash, mod_),
};

MLUA_OPEN_MODULE(mlua.platform) {
    mlua_require(ls, "mlua.int64", false);
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
