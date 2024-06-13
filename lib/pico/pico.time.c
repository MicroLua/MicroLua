// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>

#include "hardware/timer.h"
#include "pico/time.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/platform.h"
#include "mlua/thread.h"
#include "mlua/util.h"

static inline void push_absolute_time(lua_State* ls, absolute_time_t t) {
    mlua_push_int64(ls, to_us_since_boot(t));
}

static inline absolute_time_t check_time(lua_State* ls, int arg) {
    return from_us_since_boot(mlua_check_time(ls, arg));
}

static int mod_get_absolute_time_int(lua_State* ls) {
    return lua_pushinteger(ls, mlua_ticks()), 1;
}

static int mod_make_timeout_time_us(lua_State* ls) {
    uint64_t delay = mlua_check_int64(ls, 1);
    return mlua_push_deadline(ls, delay), 1;
}

static int mod_make_timeout_time_ms(lua_State* ls) {
    lua_Unsigned delay_ms = luaL_checkinteger(ls, 1);
#if MLUA_IS64INT
    lua_pushinteger(ls, mlua_timeout_time(mlua_ticks(), delay_ms * 1000));
#else
    if (delay_ms <= LUA_MAXINTEGER / 1000) {
        lua_pushinteger(ls, mlua_ticks() + delay_ms * 1000);
    } else {
        mlua_push_int64(ls, to_us_since_boot(make_timeout_time_ms(delay_ms)));
    }
#endif
    return 1;
}

static int mod_sleep_until_1(lua_State* ls, int status, lua_KContext ctx);

static int mod_sleep_until(lua_State* ls) {
    absolute_time_t t = check_time(ls, 1);
    if (!mlua_thread_blocking(ls)) {
        if (time_reached(t)) return 0;
        return mlua_thread_suspend(ls, &mod_sleep_until_1, 0, 1);
    }
    sleep_until(t);
    return 0;
}

static int mod_sleep_until_1(lua_State* ls, int status, lua_KContext ctx) {
    if (time_reached(from_us_since_boot(mlua_to_time(ls, 1)))) return 0;
    return mlua_thread_suspend(ls, &mod_sleep_until_1, 0, 1);
}

static int mod_sleep_us(lua_State* ls) {
    uint64_t delay = mlua_check_int64(ls, 1);
    if (delay == 0) return 0;
    if (!mlua_thread_blocking(ls)) {
        lua_settop(ls, 0);
        mlua_push_deadline(ls, delay);
        return mlua_thread_suspend(ls, &mod_sleep_until_1, 0, 1);
    }
    sleep_us(delay);
    return 0;
}

static int mod_sleep_ms(lua_State* ls) {
    lua_Unsigned delay = luaL_checkinteger(ls, 1);
    if (delay == 0) return 0;
    if (!mlua_thread_blocking(ls)) {
        lua_settop(ls, 0);
#if MLUA_IS64INT
        lua_pushinteger(ls, mlua_timeout_time(mlua_ticks(), delay * 1000));
#else
        if (delay <= LUA_MAXINTEGER / 1000) {
            lua_pushinteger(ls, mlua_ticks() + delay * 1000);
        } else {
            mlua_push_int64(ls, to_us_since_boot(make_timeout_time_ms(delay)));
        }
#endif
        return mlua_thread_suspend(ls, &mod_sleep_until_1, 0, 1);
    }
    sleep_ms(delay);
    return 0;
}

static int alarm_thread_1(lua_State* ls, int status, lua_KContext ctx);
static int alarm_thread_2(lua_State* ls, int status, lua_KContext ctx);

static int alarm_thread(lua_State* ls) {
    lua_pushcfunction(ls, &mod_sleep_until);
    lua_pushvalue(ls, lua_upvalueindex(1));  // time
    return mlua_callk(ls, 1, 0, alarm_thread_1, 0);
}

static int alarm_thread_1(lua_State* ls, int status, lua_KContext ctx) {
    lua_pushvalue(ls, lua_upvalueindex(2));  // callback
    lua_pushthread(ls);
    return mlua_callk(ls, 1, 1, alarm_thread_2, ctx);
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
    absolute_time_t now = get_absolute_time();
    absolute_time_t next = delay < 0 ?
        delayed_by_us(from_us_since_boot(mlua_to_time(ls, lua_upvalueindex(1))),
                      (uint64_t)-delay)
        : delayed_by_us(now, (uint64_t)delay);
    int64_t dt = absolute_time_diff_us(now, next);
    if (LUA_MININTEGER <= dt && dt <= LUA_MAXINTEGER) {
        lua_pushinteger(ls, (lua_Unsigned)to_us_since_boot(next));
    } else {
        mlua_push_int64(ls, to_us_since_boot(next));
    }
    lua_replace(ls, lua_upvalueindex(1));
    return alarm_thread(ls);
}

typedef enum Delta {
    D_UNKNOWN,
    D_INTEGER,
    D_INT64,
} Delta;

static int schedule_alarm(lua_State* ls, absolute_time_t time,
                          bool fire_if_past, int64_t delay, Delta delta) {
    if (!fire_if_past && time_reached(time)) return 0;

    // Start the alarm thread.
    if (delta == D_INTEGER
        || (delta == D_UNKNOWN
            && absolute_time_diff_us(get_absolute_time(), time)
               <= LUA_MAXINTEGER)) {
        lua_pushinteger(ls, (lua_Unsigned)to_us_since_boot(time));  // time
    } else {
        mlua_push_int64(ls, to_us_since_boot(time));  // time
    }
    lua_pushvalue(ls, 2);  // callback
    mlua_push_minint(ls, delay);  // delay
    lua_pushcclosure(ls, &alarm_thread, 3);
    mlua_thread_start(ls);
    return 1;
}

static int mod_add_alarm_at(lua_State* ls) {
    absolute_time_t time = check_time(ls, 1);
    bool fire_if_past = mlua_to_cbool(ls, 3);
    return schedule_alarm(ls, time, fire_if_past, 0, D_UNKNOWN);
}

static int mod_add_alarm_in_us(lua_State* ls) {
    uint64_t delay = mlua_check_int64(ls, 1);
    bool fire_if_past = mlua_to_cbool(ls, 3);
    return schedule_alarm(ls, make_timeout_time_us(delay), fire_if_past, 0,
                          delay <= LUA_MAXINTEGER ? D_INTEGER : D_INT64);
}

static int mod_add_alarm_in_ms(lua_State* ls) {
    uint32_t delay = luaL_checkinteger(ls, 1);
    bool fire_if_past = mlua_to_cbool(ls, 3);
    return schedule_alarm(ls, make_timeout_time_ms(delay), fire_if_past, 0,
                          delay <= LUA_MAXINTEGER / 1000 ? D_INTEGER : D_INT64);
}

static int mod_cancel_alarm(lua_State* ls) {
    mlua_check_thread(ls, 1);
    lua_settop(ls, 1);
    mlua_thread_kill(ls);
    return 1;
}

static int mod_add_repeating_timer_us(lua_State* ls) {
    int64_t delay = mlua_check_int64(ls, 1);
    if (delay == 0) delay = 1;
    int64_t delta = delay >= 0 ? delay : -delay;
    return schedule_alarm(ls, make_timeout_time_us(delta), true, delay,
                          delta <= LUA_MAXINTEGER ? D_INTEGER : D_INT64);
}

static int mod_add_repeating_timer_ms(lua_State* ls) {
    int64_t delay = (int64_t)luaL_checkinteger(ls, 1) * 1000;
    if (delay == 0) delay = 1;
    int64_t delta = delay >= 0 ? delay : -delay;
    return schedule_alarm(ls, make_timeout_time_us(delta), true, delay,
                          delta <= LUA_MAXINTEGER / 1000 ? D_INTEGER : D_INT64);
}

MLUA_FUNC_R0(mod_,, get_absolute_time, push_absolute_time)
MLUA_FUNC_R1(mod_,, to_ms_since_boot, lua_pushinteger, check_time)
MLUA_FUNC_R2(mod_,, delayed_by_us, push_absolute_time, check_time,
             mlua_check_int64)
MLUA_FUNC_R2(mod_,, delayed_by_ms, push_absolute_time, check_time,
             luaL_checkinteger)
MLUA_FUNC_R2(mod_,, absolute_time_diff_us, mlua_push_minint, check_time,
             check_time)
MLUA_FUNC_R2(mod_,, absolute_time_min, push_absolute_time, check_time,
             check_time)
MLUA_FUNC_R1(mod_,, is_at_the_end_of_time, lua_pushboolean, check_time)
MLUA_FUNC_R1(mod_,, is_nil_time, lua_pushboolean, check_time)
MLUA_FUNC_R1(mod_,, best_effort_wfe_or_timeout, lua_pushboolean, check_time)

#define mod_cancel_repeating_timer mod_cancel_alarm

MLUA_SYMBOLS(module_syms) = {
    // to_us_since_boot: not useful in Lua
    // update_us_since_boot: not useful in Lua
    // from_us_since_boot: not useful in Lua
    MLUA_SYM_F(get_absolute_time, mod_),
    MLUA_SYM_F(get_absolute_time_int, mod_),
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
    MLUA_SYM_F_THREAD(add_alarm_at, mod_),
    MLUA_SYM_F_THREAD(add_alarm_in_us, mod_),
    MLUA_SYM_F_THREAD(add_alarm_in_ms, mod_),
    MLUA_SYM_F_THREAD(cancel_alarm, mod_),
    MLUA_SYM_F_THREAD(add_repeating_timer_us, mod_),
    MLUA_SYM_F_THREAD(add_repeating_timer_ms, mod_),
    MLUA_SYM_F_THREAD(cancel_repeating_timer, mod_),
};

MLUA_OPEN_MODULE(pico.time) {
    mlua_thread_require(ls);
    mlua_require(ls, "mlua.int64", false);

    mlua_new_module(ls, 0, module_syms);
    push_absolute_time(ls, at_the_end_of_time);
    lua_setfield(ls, -2, "at_the_end_of_time");
    push_absolute_time(ls, nil_time);
    lua_setfield(ls, -2, "nil_time");
    return 1;
}
