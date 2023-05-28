#include "pico/time.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/int64.h"
#include "mlua/util.h"

static absolute_time_t check_absolute_time(lua_State* ls, int arg) {
    return from_us_since_boot(mlua_check_int64(ls, arg));
}

static void push_absolute_time(lua_State* ls, absolute_time_t t) {
    mlua_push_int64(ls, to_us_since_boot(t));
}

static void push_at_the_end_of_time(lua_State* ls, mlua_reg const* reg,
                                    int nup) {
    push_absolute_time(ls, at_the_end_of_time);
}

static void push_nil_time(lua_State* ls, mlua_reg const* reg, int nup) {
    push_absolute_time(ls, nil_time);
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
MLUA_FUNC_0_1(mod_,, sleep_until, check_absolute_time)
MLUA_FUNC_0_1(mod_,, sleep_us, mlua_check_int64)
MLUA_FUNC_0_1(mod_,, sleep_ms, luaL_checkinteger)
MLUA_FUNC_1_1(mod_,, best_effort_wfe_or_timeout, lua_pushboolean,
              check_absolute_time)

static mlua_reg const module_regs[] = {
    MLUA_REG_PUSH(at_the_end_of_time, push_at_the_end_of_time),
    MLUA_REG_PUSH(nil_time, push_nil_time),
    // X(DEFAULT_ALARM_POOL_DISABLED),
    // X(DEFAULT_ALARM_POOL_HARDWARE_ALARM_NUM),
    // X(DEFAULT_ALARM_POOL_MAX_TIMERS),
#define X(n) MLUA_REG(function, n, mod_ ## n)
    // to_us_since_boot: not useful in Lua
    // update_us_since_boot: not useful in Lua
    // from_us_since_boot: not useful in Lua
    X(get_absolute_time),
    X(to_ms_since_boot),
    X(delayed_by_us),
    X(delayed_by_ms),
    X(make_timeout_time_us),
    X(make_timeout_time_ms),
    X(absolute_time_diff_us),
    X(absolute_time_min),
    X(is_at_the_end_of_time),
    X(is_nil_time),
    X(sleep_until),
    X(sleep_us),
    X(sleep_ms),
    X(best_effort_wfe_or_timeout),
    // X(alarm_pool_init_default),
    // X(alarm_pool_get_default),
    // X(alarm_pool_create),
    // X(alarm_pool_create_with_unused_hardware_alarm),
    // X(alarm_pool_hardware_alarm_num),
    // X(alarm_pool_core_num),
    // X(alarm_pool_destroy),
    // X(alarm_pool_add_alarm_at),
    // X(alarm_pool_add_alarm_at_force_in_context),
    // X(alarm_pool_add_alarm_in_us),
    // X(alarm_pool_add_alarm_in_ms),
    // X(alarm_pool_cancel_alarm),
    // TODO: The default alarm pool always executes on core 0. May need to
    //       allocate a separate pool for core 1.
    // X(add_alarm_at),
    // X(add_alarm_in_us),
    // X(add_alarm_in_ms),
    // X(cancel_alarm),
    // X(alarm_pool_add_repeating_timer_us),
    // X(alarm_pool_add_repeating_timer_ms),
    // X(add_repeating_timer_us),
    // X(add_repeating_timer_ms),
    // X(cancel_repeating_timer),
#undef X
    {NULL},
};

int luaopen_pico_time(lua_State* ls) {
    mlua_require(ls, "mlua.int64", false);
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
