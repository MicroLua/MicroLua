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

static void push_at_the_end_of_time(lua_State* ls, MLuaReg const* reg) {
    push_absolute_time(ls, at_the_end_of_time);
}

static void push_nil_time(lua_State* ls, MLuaReg const* reg) {
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

static MLuaReg const module_regs[] = {
#define MLUA_SYM(n) MLUA_REG_PUSH(n, push_ ## n)
    MLUA_SYM(at_the_end_of_time),
    MLUA_SYM(nil_time),
#undef MLUA_SYM
#define MLUA_SYM(n) {.name=#n, .push=mlua_reg_push_boolean, .boolean=n}
    //! MLUA_SYM(DEFAULT_ALARM_POOL_DISABLED),
#undef MLUA_SYM
#define MLUA_SYM(n) {.name=#n, .push=mlua_reg_push_integer, .integer=n}
    //! MLUA_SYM(DEFAULT_ALARM_POOL_HARDWARE_ALARM_NUM),
    //! MLUA_SYM(DEFAULT_ALARM_POOL_MAX_TIMERS),
#undef MLUA_SYM
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    // to_us_since_boot: not useful in Lua
    // update_us_since_boot: not useful in Lua
    // from_us_since_boot: not useful in Lua
    MLUA_SYM(get_absolute_time),
    MLUA_SYM(to_ms_since_boot),
    MLUA_SYM(delayed_by_us),
    MLUA_SYM(delayed_by_ms),
    MLUA_SYM(make_timeout_time_us),
    MLUA_SYM(make_timeout_time_ms),
    MLUA_SYM(absolute_time_diff_us),
    MLUA_SYM(absolute_time_min),
    MLUA_SYM(is_at_the_end_of_time),
    MLUA_SYM(is_nil_time),
    MLUA_SYM(sleep_until),
    MLUA_SYM(sleep_us),
    MLUA_SYM(sleep_ms),
    MLUA_SYM(best_effort_wfe_or_timeout),
    // MLUA_SYM(alarm_pool_init_default),
    // MLUA_SYM(alarm_pool_get_default),
    // MLUA_SYM(alarm_pool_create),
    // MLUA_SYM(alarm_pool_create_with_unused_hardware_alarm),
    // MLUA_SYM(alarm_pool_hardware_alarm_num),
    // MLUA_SYM(alarm_pool_core_num),
    // MLUA_SYM(alarm_pool_destroy),
    // MLUA_SYM(alarm_pool_add_alarm_at),
    // MLUA_SYM(alarm_pool_add_alarm_at_force_in_context),
    // MLUA_SYM(alarm_pool_add_alarm_in_us),
    // MLUA_SYM(alarm_pool_add_alarm_in_ms),
    // MLUA_SYM(alarm_pool_cancel_alarm),
    // MLUA_SYM(add_alarm_at),
    // MLUA_SYM(add_alarm_in_us),
    // MLUA_SYM(add_alarm_in_ms),
    // MLUA_SYM(cancel_alarm),
    // MLUA_SYM(alarm_pool_add_repeating_timer_us),
    // MLUA_SYM(alarm_pool_add_repeating_timer_ms),
    // MLUA_SYM(add_repeating_timer_us),
    // MLUA_SYM(add_repeating_timer_ms),
    // MLUA_SYM(cancel_repeating_timer),
#undef MLUA_SYM
};

int luaopen_pico_time(lua_State* ls) {
#if LIB_MLUA_MOD_MLUA_EVENT
    mlua_require(ls, "mlua.event", false);
#endif
    mlua_require(ls, "mlua.int64", false);

    // Create the module.
    mlua_new_table(ls, module_regs);
    return 1;
}
