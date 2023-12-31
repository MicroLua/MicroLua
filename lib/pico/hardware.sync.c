// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/sync.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

MLUA_FUNC_V0(mod_, __, sev)
MLUA_FUNC_V0(mod_, __, wfe)
MLUA_FUNC_V0(mod_, __, wfi)
MLUA_FUNC_V0(mod_, __, dmb)
MLUA_FUNC_V0(mod_, __, dsb)
MLUA_FUNC_V0(mod_, __, isb)
MLUA_FUNC_V0(mod_, __, mem_fence_acquire)
MLUA_FUNC_V0(mod_, __, mem_fence_release)
MLUA_FUNC_R0(mod_,, save_and_disable_interrupts, lua_pushinteger)
MLUA_FUNC_V1(mod_,, restore_interrupts, luaL_checkinteger)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(sev, mod_),
    MLUA_SYM_F(wfe, mod_),
    MLUA_SYM_F(wfi, mod_),
    MLUA_SYM_F(dmb, mod_),
    MLUA_SYM_F(dsb, mod_),
    MLUA_SYM_F(isb, mod_),
    MLUA_SYM_F(mem_fence_acquire, mod_),
    MLUA_SYM_F(mem_fence_release, mod_),
    MLUA_SYM_F(save_and_disable_interrupts, mod_),
    MLUA_SYM_F(restore_interrupts, mod_),
};

MLUA_OPEN_MODULE(hardware.sync) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
