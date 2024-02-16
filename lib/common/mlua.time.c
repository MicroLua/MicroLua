// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/platform.h"

#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

static int mod_ticks(lua_State* ls) {
    return lua_pushinteger(ls, mlua_ticks()), 1;
}

static int mod_ticks64(lua_State* ls) {
    return mlua_push_int64(ls, mlua_ticks64()), 1;
}

static int mod_to_ticks64(lua_State* ls) {
    lua_Unsigned t = luaL_checkinteger(ls, 1);
    return mlua_push_int64(ls, mlua_to_ticks64(t,
        luaL_opt(ls, (uint64_t)mlua_check_int64, 2, mlua_ticks64()))), 1;
}

static int mod_sleep_until_1(lua_State* ls, int status, lua_KContext ctx);

static int mod_sleep_until(lua_State* ls) {
    if (!mlua_thread_blocking(ls)) {
        luaL_argexpected(ls, mlua_is_time(ls, 1), 1, "integer or Int64");
        if (mlua_time_reached(ls, 1)) return 0;
        return mlua_thread_suspend(ls, &mod_sleep_until_1, 0, 1);
    }
    uint64_t time = mlua_check_time(ls, 1);
    while (!mlua_wait(time)) {}
    return 0;
}

static int mod_sleep_until_1(lua_State* ls, int status, lua_KContext ctx) {
    if (mlua_time_reached(ls, 1)) return 0;
    return mlua_thread_suspend(ls, &mod_sleep_until_1, 0, 1);
}

static int mod_sleep_for(lua_State* ls) {
    int64_t delay = mlua_check_int64(ls, 1);
    if (delay <= 0) return 0;
    lua_settop(ls, 0);
    if (delay <= LUA_MAXINTEGER) {
        lua_pushinteger(ls, mlua_ticks() + (lua_Unsigned)delay);
    } else {
        uint64_t t = mlua_ticks64() + delay;
        uint64_t min, max;
        mlua_ticks_range(&min, &max);
        if (t > max) t = max;
        mlua_push_int64(ls, t);
    }
    return mod_sleep_until(ls);
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(ticks_per_second, integer, 1000000),

    MLUA_SYM_F(ticks, mod_),
    MLUA_SYM_F(ticks64, mod_),
    MLUA_SYM_F(to_ticks64, mod_),
    MLUA_SYM_F(sleep_until, mod_),
    MLUA_SYM_F(sleep_for, mod_),
};

MLUA_OPEN_MODULE(mlua.time) {
    mlua_require(ls, "mlua.int64", false);
    mlua_new_module(ls, 0, module_syms);
    uint64_t min, max;
    mlua_ticks_range(&min, &max);
    mlua_push_int64(ls, min);
    lua_setfield(ls, -2, "min_ticks");
    mlua_push_int64(ls, max);
    lua_setfield(ls, -2, "max_ticks");
    return 1;
}
