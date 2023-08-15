#include <stdbool.h>

#include "pico/stdio.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/util.h"

#if LIB_MLUA_MOD_MLUA_EVENT

typedef struct AvailableState {
    MLuaEvent event;
    bool pending;
} AvailableState;

static AvailableState chars_available_state = {.event = MLUA_EVENT_UNSET};

static void __time_critical_func(chars_available)(void* ud) {
    uint32_t save = mlua_event_lock();
    chars_available_state.pending = true;
    mlua_event_unlock(save);
    mlua_event_set(chars_available_state.event);
}

static int mod_enable_chars_available(lua_State* ls) {
    if (lua_type(ls, 1) == LUA_TBOOLEAN && !lua_toboolean(ls, 1)) {
        stdio_set_chars_available_callback(NULL, NULL);
        mlua_event_unclaim(ls, &chars_available_state.event);
        return 0;
    }
    char const* err = mlua_event_claim(&chars_available_state.event);
    if (err != NULL) return luaL_error(ls, "stdio: %s", err);
    uint32_t save = mlua_event_lock();
    chars_available_state.pending = false;
    mlua_event_unlock(save);
    stdio_set_chars_available_callback(&chars_available, NULL);
    return 0;
}

static int try_pending(lua_State* ls, bool timeout) {
    uint32_t save = mlua_event_lock();
    bool pending = chars_available_state.pending;
    chars_available_state.pending = false;
    mlua_event_unlock(save);
    return pending ? 0 : -1;
}

static int mod_wait_chars_available(lua_State* ls) {
    return mlua_event_wait(ls, chars_available_state.event, &try_pending, 0);
}

#endif  // LIB_MLUA_MOD_MLUA_EVENT

MLUA_FUNC_1_0(mod_, stdio_, init_all, lua_pushboolean)
MLUA_FUNC_0_0(mod_, stdio_, flush)
MLUA_FUNC_0_2(mod_, stdio_, set_driver_enabled, mlua_check_userdata,
              mlua_to_cbool)
MLUA_FUNC_0_1(mod_, stdio_, filter_driver, mlua_check_userdata)
MLUA_FUNC_0_2(mod_, stdio_, set_translate_crlf, mlua_check_userdata,
              mlua_to_cbool)
MLUA_FUNC_1_1(mod_,, getchar_timeout_us, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_1_1(mod_,, putchar_raw, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_1_1(mod_,, puts_raw, lua_pushinteger, luaL_checkstring)

static MLuaSym const module_syms[] = {
    //! MLUA_SYM_V(ENABLE_CRLF_SUPPORT, boolean, PICO_STDIO_ENABLE_CRLF_SUPPORT),
    //! MLUA_SYM_V(DEFAULT_CRLF, boolean, PICO_STDIO_DEFAULT_CRLF),

    MLUA_SYM_F(init_all, mod_),
    MLUA_SYM_F(flush, mod_),
    MLUA_SYM_F(getchar_timeout_us, mod_),
    MLUA_SYM_F(set_driver_enabled, mod_),
    MLUA_SYM_F(filter_driver, mod_),
    MLUA_SYM_F(set_translate_crlf, mod_),
    MLUA_SYM_F(putchar_raw, mod_),
    MLUA_SYM_F(puts_raw, mod_),
    // set_chars_available_callback: See enable_chars_available, wait_chars_available
#if LIB_MLUA_MOD_MLUA_EVENT
    MLUA_SYM_F(enable_chars_available, mod_),
    MLUA_SYM_F(wait_chars_available, mod_),
#endif
};

int luaopen_pico_stdio(lua_State* ls) {
    mlua_event_require(ls);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
