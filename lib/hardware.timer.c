#include <string.h>

#include "hardware/sync.h"
#include "hardware/timer.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/int64.h"
#include "mlua/util.h"

static uint check_alarm(lua_State* ls, int index) {
    lua_Integer alarm = luaL_checkinteger(ls, index);
    luaL_argcheck(ls, 0 <= alarm && alarm < (lua_Integer)NUM_TIMERS, index,
                  "invalid alarm number");
    return alarm;
}

static absolute_time_t check_absolute_time(lua_State* ls, int arg) {
    return from_us_since_boot(mlua_check_int64(ls, arg));
}

typedef struct AlarmState {
    MLuaEvent events[NUM_TIMERS];
    uint8_t pending;
} AlarmState;

static_assert(NUM_TIMERS <= 8 * sizeof(uint8_t), "pending bitmask too small");

static AlarmState alarm_state[NUM_CORES];

static void __time_critical_func(handle_alarm)(uint alarm) {
    AlarmState* state = &alarm_state[get_core_num()];
    state->pending |= 1u << alarm;
    mlua_event_set(state->events[alarm], true);
}

static int mod_enable_alarm(lua_State* ls) {
    uint alarm = check_alarm(ls, 1);
    MLuaEvent* ev = &alarm_state[get_core_num()].events[alarm];
    if (lua_type(ls, 2) == LUA_TBOOLEAN && !lua_toboolean(ls, 2)) {
        hardware_alarm_set_callback(alarm, NULL);
        mlua_event_unclaim(ls, ev);
        return 0;
    }
    mlua_event_claim(ls, ev);
    hardware_alarm_set_callback(alarm, &handle_alarm);
    return 0;
}

static bool get_pending(AlarmState* state, uint alarm) {
    uint8_t mask = 1u << alarm;
    uint32_t save = save_and_disable_interrupts();
    uint8_t pending = state->pending;
    state->pending &= ~mask;
    restore_interrupts(save);
    return (pending & mask) != 0;
}

static int mod_wait_alarm_1(lua_State* ls);
static int mod_wait_alarm_2(lua_State* ls, int status, lua_KContext ctx);

static int mod_wait_alarm(lua_State* ls) {
    uint alarm = check_alarm(ls, 1);
    AlarmState* state = &alarm_state[get_core_num()];
    if (get_pending(state, alarm)) return 0;
    mlua_event_watch(ls, state->events[alarm]);
    return mod_wait_alarm_1(ls);
}

static int mod_wait_alarm_1(lua_State* ls) {
    return mlua_event_suspend(ls, &mod_wait_alarm_2, 0);
}

static int mod_wait_alarm_2(lua_State* ls, int status, lua_KContext ctx) {
    uint alarm = check_alarm(ls, 1);
    AlarmState* state = &alarm_state[get_core_num()];
    if (!get_pending(state, alarm)) return mod_wait_alarm_1(ls);
    mlua_event_unwatch(ls, state->events[alarm]);
    return 0;
}

static void cancel_alarm(uint alarm) {
    hardware_alarm_cancel(alarm);
    AlarmState* state = &alarm_state[get_core_num()];
    uint32_t save = save_and_disable_interrupts();
    state->pending &= ~(1u << alarm);
    restore_interrupts(save);
}

static int mod_set_target(lua_State* ls) {
    uint alarm = check_alarm(ls, 1);
    absolute_time_t t = check_absolute_time(ls, 2);
    // Cancel first to avoid that the pending flag is set between resetting it
    // and setting the new target.
    cancel_alarm(alarm);
    lua_pushboolean(ls, hardware_alarm_set_target(alarm, t));
    return 1;
}

static int mod_cancel(lua_State* ls) {
    uint alarm = check_alarm(ls, 1);
    cancel_alarm(alarm);
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
MLUA_FUNC_0_1(mod_, hardware_alarm_, force_irq, check_alarm)

static MLuaReg const module_regs[] = {
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(time_us_32),
    MLUA_SYM(time_us_64),
    MLUA_SYM(busy_wait_us_32),
    MLUA_SYM(busy_wait_us),
    MLUA_SYM(busy_wait_ms),
    MLUA_SYM(busy_wait_until),
    MLUA_SYM(time_reached),
    MLUA_SYM(claim),
    MLUA_SYM(claim_unused),
    MLUA_SYM(unclaim),
    MLUA_SYM(is_claimed),
    // hardware_alarm_set_callback: See enable_alarm, wait_alarm
    MLUA_SYM(set_target),
    MLUA_SYM(cancel),
    MLUA_SYM(force_irq),
    MLUA_SYM(enable_alarm),
    MLUA_SYM(wait_alarm),
#undef MLUA_SYM
    {NULL},
};

int luaopen_hardware_timer(lua_State* ls) {
    mlua_require(ls, "mlua.event", false);
    mlua_require(ls, "mlua.int64", false);

    // Initialize internal state.
    AlarmState* state = &alarm_state[get_core_num()];
    uint32_t save = save_and_disable_interrupts();
    for (uint i = 0; i < NUM_TIMERS; ++i) state->events[i] = -1;
    state->pending = 0;
    restore_interrupts(save);

    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
