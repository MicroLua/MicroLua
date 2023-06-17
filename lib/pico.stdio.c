#include "pico/stdio.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/util.h"

typedef struct AvailableState {
    MLuaEvent event;
    bool pending;
} AvailableState;

static AvailableState chars_available_state[NUM_CORES];

static void __time_critical_func(chars_available)(void* ud) {
    AvailableState* state = (AvailableState*)ud;
    state->pending = true;
    mlua_event_set(state->event, true);
}

static int mod_enable_chars_available(lua_State* ls) {
    AvailableState* state = &chars_available_state[get_core_num()];
    if (lua_type(ls, 1) == LUA_TBOOLEAN && !lua_toboolean(ls, 1)) {
        stdio_set_chars_available_callback(NULL, NULL);
        mlua_event_unclaim(ls, &state->event);
        return 0;
    }
    mlua_event_claim(ls, &state->event);
    uint32_t save = save_and_disable_interrupts();
    state->pending = false;
    restore_interrupts(save);
    stdio_set_chars_available_callback(&chars_available, state);
    return 0;
}

static int try_pending(lua_State* ls) {
    AvailableState* state = &chars_available_state[get_core_num()];
    uint32_t save = save_and_disable_interrupts();
    bool pending = state->pending;
    state->pending = false;
    restore_interrupts(save);
    return pending ? 0 : -1;
}

static int mod_wait_chars_available(lua_State* ls) {
    return mlua_event_wait(ls, chars_available_state[get_core_num()].event,
                           &try_pending, 0);
}

MLUA_FUNC_1_0(mod_, stdio_, init_all, lua_pushboolean)
MLUA_FUNC_0_0(mod_, stdio_, flush)
MLUA_FUNC_1_1(mod_,, getchar_timeout_us, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_1_1(mod_,, putchar_raw, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_1_1(mod_,, puts_raw, lua_pushinteger, luaL_checkstring)

static MLuaReg const module_regs[] = {
#define MLUA_SYM(n) MLUA_REG(boolean, n, PICO_STDIO_ ## n)
    //! MLUA_SYM(ENABLE_CRLF_SUPPORT),
    //! MLUA_SYM(DEFAULT_CRLF),
#undef MLUA_SYM
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(init_all),
    MLUA_SYM(flush),
    MLUA_SYM(getchar_timeout_us),
    // MLUA_SYM(set_driver_enabled),
    // MLUA_SYM(filter_driver),
    // MLUA_SYM(set_translate_crlf),
    MLUA_SYM(putchar_raw),
    MLUA_SYM(puts_raw),
    // set_chars_available_callback: See enable_chars_available, wait_chars_available
    MLUA_SYM(enable_chars_available),
    MLUA_SYM(wait_chars_available),
#undef MLUA_SYM
    {NULL},
};

int luaopen_pico_stdio(lua_State* ls) {
    mlua_require(ls, "mlua.event", false);

    // Initialize internal state.
    AvailableState* state = &chars_available_state[get_core_num()];
    uint32_t save = save_and_disable_interrupts();
    state->event = -1;
    state->pending = false;
    restore_interrupts(save);

    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
