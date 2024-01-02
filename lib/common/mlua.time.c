// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/platform.h"

#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/util.h"

static void mod_min_ticks(lua_State* ls, MLuaSymVal const* value) {
    uint64_t min, max;
    mlua_platform_time_range(&min, &max);
    mlua_push_int64(ls, min);
}

static void mod_max_ticks(lua_State* ls, MLuaSymVal const* value) {
    uint64_t min, max;
    mlua_platform_time_range(&min, &max);
    mlua_push_int64(ls, max);
}

static int mod_ticks(lua_State* ls) {
    mlua_push_int64(ls, mlua_platform_ticks_us());
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(ticks_per_second, integer, 1000000),
    MLUA_SYM_P(min_ticks, mod_),
    MLUA_SYM_P(max_ticks, mod_),

    MLUA_SYM_F(ticks, mod_),
};

MLUA_OPEN_MODULE(mlua.time) {
    mlua_require(ls, "mlua.int64", false);
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
