#include <string.h>

#include "hardware/timer.h"
#include "hardware/sync.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/int64.h"
#include "mlua/signal.h"
#include "mlua/util.h"

static SigNum alarm_signals[NUM_CORES][NUM_TIMERS];

static void __time_critical_func(handle_alarm)(uint alarm) {
    mlua_signal_set(alarm_signals[get_core_num()][alarm], true);
}

static lua_Integer check_alarm(lua_State* ls, int index) {
    lua_Integer alarm = luaL_checkinteger(ls, index);
    luaL_argcheck(ls, 0 <= alarm && alarm < (lua_Integer)NUM_TIMERS, index,
                  "invalid alarm number");
    return alarm;
}

static absolute_time_t check_absolute_time(lua_State* ls, int arg) {
    return from_us_since_boot(mlua_check_int64(ls, arg));
}

static int mod_set_callback(lua_State* ls) {
    lua_Integer alarm = check_alarm(ls, 1);
    bool has_cb = !lua_isnoneornil(ls, 2);
    SigNum* sig = &alarm_signals[get_core_num()][alarm];
    if (*sig < 0) {  // No callback is currently set
        if (!has_cb) return 0;
        mlua_signal_claim(ls, sig, 2);
        hardware_alarm_set_callback(alarm, &handle_alarm);
    } else if (has_cb) {  // An existing callback is being updated
        mlua_signal_set_handler(ls, *sig, 2);
    } else {  // An existing callback is being unset
        hardware_alarm_set_callback(alarm, NULL);
        mlua_signal_unclaim(ls, sig);
    }
    return 0;
}

MLUA_FUNC_1_0(mod_,, time_us_32, lua_pushinteger)
MLUA_FUNC_1_0(mod_,, time_us_64, mlua_push_int64)
MLUA_FUNC_0_1(mod_,, busy_wait_us_32, luaL_checkinteger)
MLUA_FUNC_0_1(mod_,, busy_wait_us, mlua_check_int64)
MLUA_FUNC_0_1(mod_,, busy_wait_ms, luaL_checkinteger)
MLUA_FUNC_0_1(mod_,, busy_wait_until, check_absolute_time)
MLUA_FUNC_1_1(mod_,, time_reached, lua_pushboolean, check_absolute_time)
MLUA_FUNC_0_1(mod_, hardware_alarm_, claim, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, hardware_alarm_, claim_unused, lua_pushinteger,
              lua_toboolean)
MLUA_FUNC_0_1(mod_, hardware_alarm_, unclaim, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, hardware_alarm_, is_claimed, lua_pushboolean,
              luaL_checkinteger)
MLUA_FUNC_1_2(mod_, hardware_alarm_, set_target, lua_pushboolean, check_alarm,
              check_absolute_time)
MLUA_FUNC_0_1(mod_, hardware_alarm_, cancel, check_alarm)
MLUA_FUNC_0_1(mod_, hardware_alarm_, force_irq, check_alarm)

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(function, n, mod_ ## n)
    X(time_us_32),
    X(time_us_64),
    X(busy_wait_us_32),
    X(busy_wait_us),
    X(busy_wait_ms),
    X(busy_wait_until),
    X(time_reached),
    X(claim),
    X(claim_unused),
    X(unclaim),
    X(is_claimed),
    X(set_callback),
    X(set_target),
    X(cancel),
    X(force_irq),
#undef X
    {NULL},
};

int luaopen_hardware_timer(lua_State* ls) {
    mlua_require(ls, "int64", false);
    mlua_require(ls, "signal", false);

    // Initialize internal state.
    SigNum* sigs = alarm_signals[get_core_num()];
    uint32_t save = save_and_disable_interrupts();
    for (int i = 0; i < (int)NUM_TIMERS; ++i) sigs[i] = -1;
    restore_interrupts(save);

    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
