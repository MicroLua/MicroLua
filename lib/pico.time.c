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
    // TODO: MLUA_SYM_F(add_alarm_at, mod_),
    // TODO: MLUA_SYM_F(add_alarm_in_us, mod_),
    // TODO: MLUA_SYM_F(add_alarm_in_ms, mod_),
    // TODO: MLUA_SYM_F(cancel_alarm, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_add_repeating_timer_us, mod_),
    // TODO: MLUA_SYM_F(alarm_pool_add_repeating_timer_ms, mod_),
    // TODO: MLUA_SYM_F(add_repeating_timer_us, mod_),
    // TODO: MLUA_SYM_F(add_repeating_timer_ms, mod_),
    // TODO: MLUA_SYM_F(cancel_repeating_timer, mod_),
};

int luaopen_pico_time(lua_State* ls) {
    mlua_event_require(ls);
    mlua_require(ls, "mlua.int64", false);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
