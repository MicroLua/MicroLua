// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/rtc.h"

#include <stdbool.h>

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/module.h"
#include "mlua/util.h"

static lua_Integer get_datetime_field(lua_State* ls, int arg,
                                      char const* name) {
    switch (lua_getfield(ls, arg, name)) {
    case LUA_TNIL:
        lua_pop(ls, 1);
        return -1;
    case LUA_TNUMBER:
        int isnum;
        lua_Integer v = lua_tointegerx(ls, -1, &isnum);
        lua_pop(ls, 1);
        if (isnum) return v;
        __attribute__((fallthrough));
    default:
        return luaL_error(ls, "invalid datetime %s value", name);
    }
}

static void check_datetime(lua_State* ls, int arg, datetime_t* dt) {
    luaL_argexpected(ls, lua_istable(ls, arg), arg, "datetime table");
    dt->year = get_datetime_field(ls, arg, "year");
    dt->month = get_datetime_field(ls, arg, "month");
    dt->day = get_datetime_field(ls, arg, "day");
    dt->dotw = get_datetime_field(ls, arg, "dotw");
    dt->hour = get_datetime_field(ls, arg, "hour");
    dt->min = get_datetime_field(ls, arg, "min");
    dt->sec = get_datetime_field(ls, arg, "sec");
}

static int mod_set_datetime(lua_State* ls) {
    datetime_t dt;
    check_datetime(ls, 1, &dt);
    lua_pushboolean(ls, rtc_set_datetime(&dt));
    return 1;
}

static void set_datetime_field(lua_State* ls, char const* name,
                               lua_Integer value) {
    if (value < 0) return;
    lua_pushinteger(ls, value);
    lua_setfield(ls, -2, name);
}

static int mod_get_datetime(lua_State* ls) {
    datetime_t dt;
    if (!rtc_get_datetime(&dt)) return 0;
    lua_newtable(ls);
    set_datetime_field(ls, "year", dt.year);
    set_datetime_field(ls, "month", dt.month);
    set_datetime_field(ls, "day", dt.day);
    set_datetime_field(ls, "dotw", dt.dotw);
    set_datetime_field(ls, "hour", dt.hour);
    set_datetime_field(ls, "min", dt.min);
    set_datetime_field(ls, "sec", dt.sec);
    return 1;
}

#if LIB_MLUA_MOD_MLUA_THREAD

typedef struct RtcState {
    MLuaEvent event;
    bool pending;
} RtcState;

static RtcState rtc_state;

static void handle_alarm() {
    mlua_event_lock();
    rtc_state.pending = true;
    mlua_event_set_nolock(&rtc_state.event);
    mlua_event_unlock();
}

static int handle_alarm_event(lua_State* ls) {
    mlua_event_lock();
    bool pending = rtc_state.pending;
    rtc_state.pending = false;
    mlua_event_unlock();
    if (pending) {  // Call the callback
        lua_rawgetp(ls, LUA_REGISTRYINDEX, &rtc_state.pending);
        lua_callk(ls, 0, 0, 0, &mlua_cont_return_ctx);
    }
    return 0;
}

static int alarm_handler_done(lua_State* ls) {
    lua_pushnil(ls);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, &rtc_state.pending);
    datetime_t dt = {.year = 4095, .month = -1, .day = -1, .dotw = -1,
                     .hour = -1, .min = -1, .sec = -1};
    rtc_set_alarm(&dt, NULL);
    rtc_disable_alarm();
    mlua_event_disable(ls, &rtc_state.event);
    return 0;
}

static int mod_set_alarm(lua_State* ls) {
    if (lua_isnil(ls, 2)) {  // Nil callback, kill the handler thread
        mlua_event_stop_handler(ls, &rtc_state.event);
        return 0;
    }
    datetime_t dt;
    check_datetime(ls, 1, &dt);

    // Set the alarm and callback.
    bool enabled = mlua_event_enable(ls, &rtc_state.event);
    if (enabled) {
        if (lua_isnone(ls, 2)) {  // No handler
            mlua_event_disable(ls, &rtc_state.event);
            return 0;
        }
    }
    mlua_event_lock();
    rtc_state.pending = false;
    rtc_set_alarm(&dt, &handle_alarm);
    mlua_event_unlock();

    // Set the callback handler.
    if (!lua_isnone(ls, 2)) {
        lua_settop(ls, 2);  // handler
        lua_rawsetp(ls, LUA_REGISTRYINDEX, &rtc_state.pending);
    }

    // Start the event handler thread if it isn't already running.
    if (!enabled) {
        mlua_event_push_handler_thread(ls, &rtc_state.event);
        return 1;
    }
    lua_pushcfunction(ls, &handle_alarm_event);
    lua_pushcfunction(ls, &alarm_handler_done);
    return mlua_event_handle(ls, &rtc_state.event, &mlua_cont_return_ctx, 1);
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

static int mod_disable_alarm(lua_State* ls) {
    rtc_disable_alarm();
#if LIB_MLUA_MOD_MLUA_THREAD
    mlua_event_lock();
    rtc_state.pending = false;
    mlua_event_unlock();
#endif
    return 0;
}

MLUA_FUNC_V0(mod_, rtc_, init)
MLUA_FUNC_R0(mod_, rtc_, running, lua_pushboolean)
MLUA_FUNC_V0(mod_, rtc_, enable_alarm)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(init, mod_),
    MLUA_SYM_F(set_datetime, mod_),
    MLUA_SYM_F(get_datetime, mod_),
    MLUA_SYM_F(running, mod_),
#if LIB_MLUA_MOD_MLUA_THREAD
    MLUA_SYM_F(set_alarm, mod_),
#else
    MLUA_SYM_V(set_alarm, boolean, false),
#endif
    MLUA_SYM_F(enable_alarm, mod_),
    MLUA_SYM_F(disable_alarm, mod_),
};

MLUA_OPEN_MODULE(hardware.rtc) {
    mlua_event_require(ls);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
