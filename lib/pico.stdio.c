// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/pico.stdio.h"

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "pico.h"
#include "pico/stdio.h"
#include "pico/time.h"

#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/util.h"

#if LIB_MLUA_MOD_MLUA_EVENT

typedef struct StdioState {
    MLuaEvent event;
    bool pending;
} StdioState;

static StdioState stdio_state = {.event = MLUA_EVENT_UNSET};

static void __time_critical_func(handle_chars_available)(void* ud) {
    uint32_t save = mlua_event_lock();
    stdio_state.pending = true;
    mlua_event_set_nolock(stdio_state.event);
    mlua_event_unlock(save);
}

static bool chars_available_reset() {
    uint32_t save = mlua_event_lock();
    bool pending = stdio_state.pending;
    stdio_state.pending = false;
    mlua_event_unlock(save);
    return pending;
}

static void enable_chars_available(lua_State* ls) {
    char const* err = mlua_event_claim(&stdio_state.event);
    if (err == NULL) {
        uint32_t save = mlua_event_lock();
        stdio_state.pending = false;
        mlua_event_unlock(save);
        stdio_set_chars_available_callback(&handle_chars_available, NULL);
    } else if (err != mlua_event_err_already_claimed) {
        luaL_error(ls, "stdio: %s", err);
        return;
    }
}

static int mod_enable_chars_available(lua_State* ls) {
    if (lua_type(ls, 1) == LUA_TBOOLEAN && !lua_toboolean(ls, 1)) {
        stdio_set_chars_available_callback(NULL, NULL);
        mlua_event_unclaim(ls, &stdio_state.event);
        return 0;
    }
    enable_chars_available(ls);
    return 0;
}

static int handle_chars_available_event(lua_State* ls) {
    uint32_t save = mlua_event_lock();
    bool pending = stdio_state.pending;
    mlua_event_unlock(save);
    if (pending) {  // Call the callback
        lua_rawgetp(ls, LUA_REGISTRYINDEX, &stdio_state.pending);
        lua_callk(ls, 0, 0, 0, &mlua_cont_return_ctx);
    }
    return 0;
}

static int chars_available_handler_done(lua_State* ls) {
    lua_pushnil(ls);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, &stdio_state.pending);
    return 0;
}

static int mod_set_chars_available_callback(lua_State* ls) {
    if (lua_isnoneornil(ls, 1)) {  // Nil callback, kill the handler thread
        mlua_event_stop_handler(ls, &stdio_state.event);
        return 0;
    }

    // Set the callback.
    enable_chars_available(ls);

    // Set the callback handler.
    lua_settop(ls, 1);  // handler
    lua_rawsetp(ls, LUA_REGISTRYINDEX, &stdio_state.pending);

    // Start the event handler thread if it isn't already running.
    //
    // The chars_available callback is intentionally left enabled when the
    // thread terminates, as it may be used by other functions in this module.
    // It can be disabled manually with enable_chars_available(false).
    mlua_event_push_handler_thread(ls, &stdio_state.event);
    if (!lua_isnil(ls, -1)) return 1;
    lua_pop(ls, 1);
    lua_pushcfunction(ls, &handle_chars_available_event);
    lua_pushcfunction(ls, &chars_available_handler_done);
    return mlua_event_handle(ls, &stdio_state.event, &mlua_cont_return_ctx, 1);
}

#endif  // LIB_MLUA_MOD_MLUA_EVENT

static int try_getchar(lua_State* ls, bool timeout) {
    if (timeout) {
        lua_pushinteger(ls, PICO_ERROR_TIMEOUT);
        return 1;
    }
    if (!chars_available_reset()) return -1;
    lua_pushinteger(ls, getchar());
    return 1;
}

static int mod_getchar(lua_State* ls) {
    if (mlua_event_can_wait(&stdio_state.event)) {
        return mlua_event_wait(ls, stdio_state.event, &try_getchar, 0);
    }
    lua_pushinteger(ls, getchar());
    return 1;
}

static int mod_getchar_timeout_us(lua_State* ls) {
    uint32_t timeout = luaL_checkinteger(ls, 1);
    if (mlua_event_can_wait(&stdio_state.event)) {
        mlua_push_int64(ls, to_us_since_boot(make_timeout_time_us(timeout)));
        lua_replace(ls, 1);
        return mlua_event_wait(ls, stdio_state.event, &try_getchar, 1);
    }
    lua_pushinteger(ls, getchar_timeout_us(timeout));
    return 1;
}

static int do_read(lua_State* ls, int fd, lua_Integer len) {
    luaL_Buffer buf;
    char* p = luaL_buffinitsize(ls, &buf, len);
    int cnt = read(fd, p, len);
    if (cnt < 0) return luaL_fileresult(ls, 0, NULL);
    luaL_pushresultsize(&buf, cnt);
    return 1;
}

static int try_read(lua_State* ls, bool timeout) {
    if (!chars_available_reset()) return -1;
    return do_read(ls, lua_tointeger(ls, -2), lua_tointeger(ls, -1));
}

int mlua_pico_stdio_read(lua_State* ls, int fd, int index) {
    lua_Integer len = luaL_checkinteger(ls, index);
    luaL_argcheck(ls, 0 <= len, index, "invalid length");
    if (mlua_event_can_wait(&stdio_state.event)) {
        lua_pushinteger(ls, fd);
        lua_pushinteger(ls, len);
        return mlua_event_wait(ls, stdio_state.event, &try_read, 0);
    }
    return do_read(ls, fd, len);
}

static int mod_read(lua_State* ls) {
    return mlua_pico_stdio_read(ls, STDIN_FILENO, 1);
}

int mlua_pico_stdio_write(lua_State* ls, int fd, int index) {
    size_t len;
    char const* s = luaL_checklstring(ls, index, &len);
    int cnt = write(fd, s, len);
    if (cnt < 0) return luaL_fileresult(ls, 0, NULL);
    lua_pushinteger(ls, cnt);
    return 1;
}

static int mod_write(lua_State* ls) {
    return mlua_pico_stdio_write(ls, STDOUT_FILENO, 1);
}

MLUA_FUNC_1_0(mod_, stdio_, init_all, lua_pushboolean)
MLUA_FUNC_0_0(mod_, stdio_, flush)
MLUA_FUNC_0_2(mod_, stdio_, set_driver_enabled, mlua_check_userdata,
              mlua_to_cbool)
MLUA_FUNC_0_1(mod_, stdio_, filter_driver, mlua_check_userdata_or_nil)
MLUA_FUNC_0_2(mod_, stdio_, set_translate_crlf, mlua_check_userdata,
              mlua_to_cbool)
MLUA_FUNC_1_1(mod_,, putchar, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_1_1(mod_,, putchar_raw, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_1_1(mod_,, puts, lua_pushinteger, luaL_checkstring)
MLUA_FUNC_1_1(mod_,, puts_raw, lua_pushinteger, luaL_checkstring)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(EOF, integer, EOF),
    MLUA_SYM_V(ENABLE_CRLF_SUPPORT, boolean, PICO_STDIO_ENABLE_CRLF_SUPPORT),
    MLUA_SYM_V(DEFAULT_CRLF, boolean, PICO_STDIO_DEFAULT_CRLF),

    MLUA_SYM_F(init_all, mod_),
    MLUA_SYM_F(flush, mod_),
    MLUA_SYM_F(getchar, mod_),
    MLUA_SYM_F(getchar_timeout_us, mod_),
    MLUA_SYM_F(set_driver_enabled, mod_),
    MLUA_SYM_F(filter_driver, mod_),
    MLUA_SYM_F(set_translate_crlf, mod_),
    MLUA_SYM_F(putchar, mod_),
    MLUA_SYM_F(putchar_raw, mod_),
    MLUA_SYM_F(puts, mod_),
    MLUA_SYM_F(puts_raw, mod_),
#if LIB_MLUA_MOD_MLUA_EVENT
    MLUA_SYM_F(set_chars_available_callback, mod_),
#else
    MLUA_SYM_V(set_chars_available_callback, boolean, false),
#endif
    MLUA_SYM_F(read, mod_),
    MLUA_SYM_F(write, mod_),
#if LIB_MLUA_MOD_MLUA_EVENT
    MLUA_SYM_F(enable_chars_available, mod_),
#else
    MLUA_SYM_V(enable_chars_available, boolean, false),
#endif
};

MLUA_OPEN_MODULE(pico.stdio) {
    mlua_event_require(ls);
    mlua_require(ls, "mlua.int64", false);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
