// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/watchdog.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"
#include "mlua/util.h"

MLUA_FUNC_V1(mod_, watchdog_, start_tick, luaL_checkinteger)
MLUA_FUNC_V0(mod_, watchdog_, update)
MLUA_FUNC_V2(mod_, watchdog_, enable, luaL_checkinteger, mlua_to_cbool)
MLUA_FUNC_R0(mod_, watchdog_, caused_reboot, lua_pushboolean)
MLUA_FUNC_R0(mod_, watchdog_, enable_caused_reboot, lua_pushboolean)
MLUA_FUNC_R0(mod_, watchdog_, get_count, lua_pushinteger)

MLUA_SYMBOLS(module_syms) = {
    // watchdog_reboot: Not useful in Lua
    MLUA_SYM_F(start_tick, mod_),
    MLUA_SYM_F(update, mod_),
    MLUA_SYM_F(enable, mod_),
    MLUA_SYM_F(caused_reboot, mod_),
    MLUA_SYM_F(enable_caused_reboot, mod_),
    MLUA_SYM_F(get_count, mod_),
};

MLUA_OPEN_MODULE(hardware.watchdog) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
