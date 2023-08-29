#include <stdbool.h>

#include "hardware/timer.h"
#include "pico/time.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/int64.h"
#include "mlua/util.h"

static absolute_time_t check_absolute_time(lua_State* ls, int arg) {
    return from_us_since_boot(mlua_check_int64(ls, arg));
}

static void push_absolute_time(lua_State* ls, absolute_time_t t) {
    mlua_push_int64(ls, to_us_since_boot(t));
}

static void push_at_the_end_of_time(lua_State* ls, MLuaSym const* sym) {
    push_absolute_time(ls, at_the_end_of_time);
}

static void push_nil_time(lua_State* ls, MLuaSym const* sym) {
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
    push_absolute_time(ls, make_timeout_time_us(mlua_check_int64(ls, 1)));
    lua_replace(ls, 1);
    return mod_sleep_until(ls);
}

static int mod_sleep_ms(lua_State* ls) {
    push_absolute_time(ls, make_timeout_time_ms(luaL_checkinteger(ls, 1)));
    lua_replace(ls, 1);
    return mod_sleep_until(ls);
}

static char const Alarm_name[] = "pico.time.Alarm";

typedef struct Alarm {
    absolute_time_t time;
    alarm_pool_t* pool;
    MLuaEvent event;
    alarm_id_t id;
} Alarm;

static int64_t __time_critical_func(handle_alarm)(alarm_id_t id, void* ud) {
    mlua_event_set(((Alarm*)ud)->event);
    return 0;
}

static int alarm_thread_1(lua_State* ls, int status, lua_KContext ctx);
static int alarm_thread_2(lua_State* ls, int status, lua_KContext ctx);
static int alarm_thread_done(lua_State* ls);

static int alarm_thread(lua_State* ls) {
    Alarm* alarm = lua_touserdata(ls, lua_upvalueindex(1));

    // Set up a deferred to clean up on exit.
    lua_pushvalue(ls, lua_upvalueindex(1));
    lua_pushcclosure(ls, &alarm_thread_done, 1);
    lua_toclose(ls, -1);

    // Start watching the event and suspend.
    mlua_event_watch(ls, alarm->event);
    return mlua_event_suspend(ls, &alarm_thread_1, 0, 0);
}

static int alarm_thread_1(lua_State* ls, int status, lua_KContext ctx) {
    lua_pushvalue(ls, lua_upvalueindex(2));  // callback
    lua_pushvalue(ls, lua_upvalueindex(1));  // alarm
    lua_callk(ls, 1, 1, ctx, &alarm_thread_2);
    return alarm_thread_2(ls, LUA_OK, ctx);
}

static int alarm_thread_2(lua_State* ls, int status, lua_KContext ctx) {
    if (lua_isnoneornil(ls, -1)) return 0;
    int64_t repeat = mlua_check_int64(ls, -1);
    if (repeat == 0) return 0;
    Alarm* alarm = lua_touserdata(ls, lua_upvalueindex(1));
    if (repeat < 0) {
        alarm->time = delayed_by_us(alarm->time, (uint64_t)-repeat);
    } else {
        alarm->time = delayed_by_us(get_absolute_time(), (uint64_t)repeat);
    }
    alarm->id = alarm_pool_add_alarm_at(alarm->pool, alarm->time,
                                        &handle_alarm, alarm, false);
    if (alarm->id < 0) {
        return luaL_error(ls, "alarm: no slots available");
    } else if (alarm->id == 0) {  // Alarm time in the past
        return alarm_thread_1(ls, LUA_OK, ctx);
    }
    return mlua_event_suspend(ls, &alarm_thread_1, ctx, 0);
}

static int alarm_thread_done(lua_State* ls) {
    Alarm* alarm = lua_touserdata(ls, lua_upvalueindex(1));
    if (alarm->id > 0) {
        alarm_pool_cancel_alarm(alarm->pool, alarm->id);
        alarm->id = 0;
    }
    mlua_event_unclaim(ls, &alarm->event);
    lua_pushnil(ls);
    lua_setiuservalue(ls, lua_upvalueindex(1), -1);
    return 0;
}

static int schedule_alarm(lua_State* ls, absolute_time_t time,
                          alarm_pool_t* pool) {
    bool fire_if_past = mlua_to_cbool(ls, 3);

    MLuaEvent ev = MLUA_EVENT_UNSET;
    char const* err = mlua_event_claim(&ev);
    if (err != NULL) return luaL_error(ls, "alarm: %s", err);
    Alarm* alarm = lua_newuserdatauv(ls, sizeof(Alarm), 1);
    luaL_getmetatable(ls, Alarm_name);
    lua_setmetatable(ls, -2);
    alarm->time = time;
    alarm->pool = pool;
    alarm->event = ev;

    // Schedule the alarm.
    alarm_id_t id = alarm_pool_add_alarm_at(pool, time, &handle_alarm, alarm,
                                            fire_if_past);
    if (id < 0) {  // No alarm slots available
        return luaL_error(ls, "alarm: no slots available");
    } else if (id == 0 && !fire_if_past) {  // Alarm time in the past, not fired
        return 0;
    }
    alarm->id = id;

    // Start the callback thread.
    lua_pushthread(ls);
    luaL_getmetafield(ls, -1, "start");
    lua_remove(ls, -2);
    lua_pushvalue(ls, -2);  // alarm
    lua_pushvalue(ls, 2);   // callback
    lua_pushcclosure(ls, &alarm_thread, 2);
    lua_call(ls, 1, 1);
    lua_setiuservalue(ls, -2, 1);
    return 1;
}

static int Alarm_thread(lua_State* ls) {
    luaL_checkudata(ls, 1, Alarm_name);
    lua_getiuservalue(ls, 1, 1);
    return 1;
}

static int Alarm_cancel(lua_State* ls) {
    luaL_checkudata(ls, 1, Alarm_name);

    // Kill the callback thread. This will also cancel the alarm.
    if (lua_getiuservalue(ls, 1, 1) == LUA_TTHREAD) {
        luaL_getmetafield(ls, -1, "kill");
        lua_rotate(ls, -2, 1);
        lua_call(ls, 1, 0);
    }
    return 0;
}

#define Alarm___close Alarm_cancel

static MLuaSym const Alarm_syms[] = {
    MLUA_SYM_F(thread, Alarm_),
    MLUA_SYM_F(cancel, Alarm_),
    MLUA_SYM_F(__close, Alarm_),
};

static int mod_add_alarm_at(lua_State* ls) {
    absolute_time_t time = from_us_since_boot(mlua_check_int64(ls, 1));
    return schedule_alarm(ls, time, alarm_pool_get_default());
}

static int mod_add_alarm_in_us(lua_State* ls) {
    uint64_t delay = mlua_check_int64(ls, 1);
    return schedule_alarm(ls, delayed_by_us(get_absolute_time(), delay),
                          alarm_pool_get_default());
}

static int mod_add_alarm_in_ms(lua_State* ls) {
    uint32_t delay = luaL_checkinteger(ls, 1);
    return schedule_alarm(ls, delayed_by_ms(get_absolute_time(), delay),
                          alarm_pool_get_default());
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

#define mod_cancel_alarm Alarm_cancel

static MLuaSym const module_syms[] = {
    MLUA_SYM_P(at_the_end_of_time, push_),
    MLUA_SYM_P(nil_time, push_),
    //! MLUA_SYM_V(DEFAULT_ALARM_POOL_DISABLED, boolean, DEFAULT_ALARM_POOL_DISABLED),
    //! MLUA_SYM_V(DEFAULT_ALARM_POOL_HARDWARE_ALARM_NUM, integer, DEFAULT_ALARM_POOL_HARDWARE_ALARM_NUM),
    //! MLUA_SYM_V(DEFAULT_ALARM_POOL_MAX_TIMERS, integer, DEFAULT_ALARM_POOL_MAX_TIMERS),

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

    // TODO: MLUA_SYM_F(alarm_pool_init_default, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_get_default, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_create, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_create_with_unused_hardware_alarm, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_hardware_alarm_num, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_core_num, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_destroy, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_add_alarm_at, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_add_alarm_at_force_in_context, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_add_alarm_in_us, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_add_alarm_in_ms, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_cancel_alarm, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_add_repeating_timer_us, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_add_repeating_timer_ms, mod_),
#if !PICO_TIME_DEFAULT_ALARM_POOL_DISABLED
    MLUA_SYM_F(add_alarm_at, mod_),
    MLUA_SYM_F(add_alarm_in_us, mod_),
    MLUA_SYM_F(add_alarm_in_ms, mod_),
    MLUA_SYM_F(cancel_alarm, mod_),
    // TODO: MLUA_SYM_F(add_repeating_timer_us, mod_),
    // TODO: MLUA_SYM_F(add_repeating_timer_ms, mod_),
    // TODO: MLUA_SYM_F(cancel_repeating_timer, mod_),
#else
    MLUA_SYM_V(add_alarm_at, boolean, false),
    MLUA_SYM_V(add_alarm_in_us, boolean, false),
    MLUA_SYM_V(add_alarm_in_ms, boolean, false),
    MLUA_SYM_V(cancel_alarm, boolean, false),
    MLUA_SYM_V(add_repeating_timer_us, boolean, false),
    MLUA_SYM_V(add_repeating_timer_ms, boolean, false),
    MLUA_SYM_V(cancel_repeating_timer, boolean, false),
#endif
};

int luaopen_pico_time(lua_State* ls) {
    mlua_event_require(ls);
    mlua_require(ls, "mlua.int64", false);

    mlua_new_module(ls, 0, module_syms);

    // Create the Alarm class.
    mlua_new_class(ls, Alarm_name, Alarm_syms);
    lua_pop(ls, 1);

    return 1;
}
