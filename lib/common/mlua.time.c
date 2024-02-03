// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/platform.h"

#include "mlua/event.h"
#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

static void mod_min_ticks(lua_State* ls, MLuaSymVal const* value) {
    uint64_t min, max;
    mlua_ticks_range(&min, &max);
    mlua_push_int64(ls, min);
}

static void mod_max_ticks(lua_State* ls, MLuaSymVal const* value) {
    uint64_t min, max;
    mlua_ticks_range(&min, &max);
    mlua_push_int64(ls, max);
}

static int mod_ticks(lua_State* ls) {
    mlua_push_int64(ls, mlua_ticks());
    return 1;
}

static int mod_sleep_until_1(lua_State* ls, int status, lua_KContext ctx);

static int mod_sleep_until(lua_State* ls) {
    uint64_t t = mlua_check_int64(ls, 1);
    if (!mlua_thread_blocking(ls)) {
        if (mlua_ticks_reached(t)) return 0;
        return mlua_event_suspend(ls, &mod_sleep_until_1, 0, 1);
    }
    while (!mlua_wait(t)) /* do nothing */;
    return 0;
}

static int mod_sleep_until_1(lua_State* ls, int status, lua_KContext ctx) {
    if (mlua_ticks_reached(mlua_to_int64(ls, 1))) return 0;
    return mlua_event_suspend(ls, &mod_sleep_until_1, 0, 1);
}

static int mod_sleep_for(lua_State* ls) {
    uint64_t deadline = mlua_ticks() + mlua_check_int64(ls, 1);
    uint64_t ticks_min, ticks_max;
    mlua_ticks_range(&ticks_min, &ticks_max);
    if (deadline > ticks_max) deadline = ticks_max;
    lua_settop(ls, 0);
    mlua_push_int64(ls, deadline);
    return mod_sleep_until(ls);
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(ticks_per_second, integer, 1000000),
    MLUA_SYM_P(min_ticks, mod_),
    MLUA_SYM_P(max_ticks, mod_),

    MLUA_SYM_F(ticks, mod_),
    MLUA_SYM_F(sleep_until, mod_),
    MLUA_SYM_F(sleep_for, mod_),
};

MLUA_OPEN_MODULE(mlua.time) {
    mlua_require(ls, "mlua.int64", false);
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
