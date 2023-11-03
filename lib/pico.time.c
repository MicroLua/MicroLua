// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>

#include "hardware/timer.h"
#include "pico/time.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/util.h"

static absolute_time_t check_absolute_time(lua_State* ls, int arg) {
    return from_us_since_boot(mlua_check_int64(ls, arg));
}

static void push_absolute_time(lua_State* ls, absolute_time_t t) {
    mlua_push_int64(ls, to_us_since_boot(t));
}

static void push_at_the_end_of_time(lua_State* ls, MLuaSymVal const* value) {
    push_absolute_time(ls, at_the_end_of_time);
}

static void push_nil_time(lua_State* ls, MLuaSymVal const* value) {
    push_absolute_time(ls, nil_time);
}

static int mod_sleep_until_1(lua_State* ls, int status, lua_KContext ctx);

static int mod_sleep_until(lua_State* ls) {
    absolute_time_t t = check_absolute_time(ls, 1);
#if LIB_MLUA_MOD_MLUA_EVENT
    if (time_reached(t)) return 0;
    return mlua_event_suspend(ls, &mod_sleep_until_1, 0, 1);
#else
    sleep_until(t);
    return 0;
#endif
}

static int mod_sleep_until_1(lua_State* ls, int status, lua_KContext ctx) {
    return mod_sleep_until(ls);
}

static int mod_sleep_us(lua_State* ls) {
    absolute_time_t deadline = make_timeout_time_us(mlua_check_int64(ls, 1));
    lua_settop(ls, 0);
    push_absolute_time(ls, deadline);
    return mod_sleep_until(ls);
}

static int mod_sleep_ms(lua_State* ls) {
    absolute_time_t deadline = make_timeout_time_ms(luaL_checkinteger(ls, 1));
    lua_settop(ls, 0);
    push_absolute_time(ls, deadline);
    return mod_sleep_until(ls);
}

static int alarm_thread_1(lua_State* ls, int status, lua_KContext ctx);
static int alarm_thread_2(lua_State* ls, int status, lua_KContext ctx);

static int alarm_thread(lua_State* ls) {
    lua_pushcfunction(ls, &mod_sleep_until);
    lua_pushvalue(ls, lua_upvalueindex(1));  // time
    lua_callk(ls, 1, 0, 0, &alarm_thread_1);
    return alarm_thread_1(ls, LUA_OK, 0);
}

static int alarm_thread_1(lua_State* ls, int status, lua_KContext ctx) {
    lua_pushvalue(ls, lua_upvalueindex(2));  // callback
    lua_pushthread(ls);
    lua_callk(ls, 1, 1, ctx, &alarm_thread_2);
    return alarm_thread_2(ls, LUA_OK, ctx);
}

static int alarm_thread_2(lua_State* ls, int status, lua_KContext ctx) {
    int64_t delay = mlua_check_int64(ls, lua_upvalueindex(3));  // delay
    if (delay == 0) {  // Alarm
        if (lua_isnil(ls, -1)) return 0;
        delay = mlua_check_int64(ls, -1);
        lua_pop(ls, 1);
    } else if (lua_isboolean(ls, -1)) {  // Repeating timer
        if (!lua_toboolean(ls, -1)) return 0;
        lua_pop(ls, 1);
    } else if (!lua_isnil(ls, -1)) {  // Repeating timer, delay update
        delay = mlua_check_int64(ls, -1);
        lua_replace(ls, lua_upvalueindex(3));
    }
    if (delay == 0) return 0;
    mlua_push_int64(ls, to_us_since_boot(delay < 0 ?
        delayed_by_us(mlua_to_int64(ls, lua_upvalueindex(1)), (uint64_t)-delay)
        : delayed_by_us(get_absolute_time(), (uint64_t)delay)));
    lua_replace(ls, lua_upvalueindex(1));
    return alarm_thread(ls);
}

static int schedule_alarm(lua_State* ls, absolute_time_t time,
                          bool fire_if_past, int64_t delay) {
    if (!fire_if_past && time_reached(time)) return 0;

    // Start the alarm thread.
    mlua_push_int64(ls, to_us_since_boot(time));  // time
    lua_pushvalue(ls, 2);  // callback
    mlua_push_int64(ls, delay);  // delay
    lua_pushcclosure(ls, &alarm_thread, 3);
    mlua_thread_start(ls);
    return 1;
}

static int mod_add_alarm_at(lua_State* ls) {
    absolute_time_t time = from_us_since_boot(mlua_check_int64(ls, 1));
    bool fire_if_past = mlua_to_cbool(ls, 3);
    return schedule_alarm(ls, time, fire_if_past, 0);
}

static int mod_add_alarm_in_us(lua_State* ls) {
    uint64_t delay = mlua_check_int64(ls, 1);
    bool fire_if_past = mlua_to_cbool(ls, 3);
    return schedule_alarm(ls, make_timeout_time_us(delay), fire_if_past, 0);
}

static int mod_add_alarm_in_ms(lua_State* ls) {
    uint32_t delay = luaL_checkinteger(ls, 1);
    bool fire_if_past = mlua_to_cbool(ls, 3);
    return schedule_alarm(ls, make_timeout_time_ms(delay), fire_if_past, 0);
}

static int mod_cancel_alarm(lua_State* ls) {
    lua_State* thread = lua_tothread(ls, 1);
    luaL_argexpected(ls, thread != NULL, 1, "thread");
    if (!mlua_thread_is_alive(thread)) {
        lua_pushboolean(ls, false);
        return 1;
    }

    // Kill the alarm thread.
    lua_settop(ls, 1);
    mlua_thread_kill(ls);
    lua_pushboolean(ls, true);
    return 1;
}

static int mod_add_repeating_timer_us(lua_State* ls) {
    int64_t delay = mlua_check_int64(ls, 1);
    if (delay == 0) delay = 1;
    return schedule_alarm(
        ls, make_timeout_time_us(delay >= 0 ? delay : -delay), true, delay);
}

static int mod_add_repeating_timer_ms(lua_State* ls) {
    int64_t delay = luaL_checkinteger(ls, 1) * (int64_t)1000;
    if (delay == 0) delay = 1;
    return schedule_alarm(
        ls, make_timeout_time_us(delay >= 0 ? delay : -delay), true, delay);
}

MLUA_FUNC_1_0(mod_,, get_absolute_time, push_absolute_time)
MLUA_FUNC_1_1(mod_,, to_ms_since_boot, lua_pushinteger, check_absolute_time)
MLUA_FUNC_1_2(mod_,, delayed_by_us, push_absolute_time, check_absolute_time,
              mlua_check_int64)
MLUA_FUNC_1_2(mod_,, delayed_by_ms, push_absolute_time, check_absolute_time,
              luaL_checkinteger)
MLUA_FUNC_1_1(mod_,, make_timeout_time_us, push_absolute_time, mlua_check_int64)
MLUA_FUNC_1_1(mod_,, make_timeout_time_ms, push_absolute_time,
              luaL_checkinteger)
MLUA_FUNC_1_2(mod_,, absolute_time_diff_us, mlua_push_int64,
              check_absolute_time, check_absolute_time)
MLUA_FUNC_1_2(mod_,, absolute_time_min, push_absolute_time,
              check_absolute_time, check_absolute_time)
MLUA_FUNC_1_1(mod_,, is_at_the_end_of_time, lua_pushboolean,
              check_absolute_time)
MLUA_FUNC_1_1(mod_,, is_nil_time, lua_pushboolean, check_absolute_time)
MLUA_FUNC_1_1(mod_,, best_effort_wfe_or_timeout, lua_pushboolean,
              check_absolute_time)

#define mod_cancel_repeating_timer mod_cancel_alarm

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_P(at_the_end_of_time, push_),
    MLUA_SYM_P(nil_time, push_),

    // to_us_since_boot: not useful in Lua
    // update_us_since_boot: not useful in Lua
    // from_us_since_boot: not useful in Lua
    MLUA_SYM_F(get_absolute_time, mod_),
    MLUA_SYM_F(to_ms_since_boot, mod_),
    MLUA_SYM_F(delayed_by_us, mod_),
    MLUA_SYM_F(delayed_by_ms, mod_),
    MLUA_SYM_F(make_timeout_time_us, mod_),
    MLUA_SYM_F(make_timeout_time_ms, mod_),
    MLUA_SYM_F(absolute_time_diff_us, mod_),
    MLUA_SYM_F(absolute_time_min, mod_),
    MLUA_SYM_F(is_at_the_end_of_time, mod_),
    MLUA_SYM_F(is_nil_time, mod_),
    MLUA_SYM_F(sleep_until, mod_),
    MLUA_SYM_F(sleep_us, mod_),
    MLUA_SYM_F(sleep_ms, mod_),
    MLUA_SYM_F(best_effort_wfe_or_timeout, mod_),

    // alarm_pool_*: not useful in Lua, as thread-based alarms are unlimited
    MLUA_SYM_F(add_alarm_at, mod_),
    MLUA_SYM_F(add_alarm_in_us, mod_),
    MLUA_SYM_F(add_alarm_in_ms, mod_),
    MLUA_SYM_F(cancel_alarm, mod_),
    MLUA_SYM_F(add_repeating_timer_us, mod_),
    MLUA_SYM_F(add_repeating_timer_ms, mod_),
    MLUA_SYM_F(cancel_repeating_timer, mod_),
};

MLUA_OPEN_MODULE(pico.time) {
    mlua_event_require(ls);
    mlua_require(ls, "mlua.int64", false);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
