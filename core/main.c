// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/main.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/timer.h"
#include "pico/platform.h"

#include "mlua/module.h"
#include "mlua/util.h"

void mlua_writestringerror(char const* fmt, char const* param) {
    int i = 0;
    for (;;) {
        char c = fmt[i];
        if (c == '\0') {
            if (i > 0) fwrite(fmt, sizeof(char), i, stderr);
            fflush(stderr);
            return;
        } else if (c == '%' && fmt[i + 1] == 's') {
            if (i > 0) fwrite(fmt, sizeof(char), i, stderr);
            int plen = strlen(param);
            if (plen > 0) fwrite(param, sizeof(char), plen, stderr);
            fmt += i + 2;
            i = 0;
        } else {
            ++i;
        }
    }
}

static int getfield(lua_State* ls) {
    lua_gettable(ls, 1);
    return 1;
}

static int pmain(lua_State* ls) {
    // Register compiled-in modules.
    mlua_register_modules(ls);
    mlua_util_init(ls);

    // Set up stdio streams.
#ifdef LIB_MLUA_MOD_MLUA_STDIO
    mlua_require(ls, "mlua.stdio", false);
#endif

    // Import the main module and get its main function.
    lua_pushcfunction(ls, getfield);
    lua_getglobal(ls, "require");
    lua_rotate(ls, 1, -1);
    lua_call(ls, 1, 1);
    lua_rotate(ls, 1, -1);
    bool has_main = lua_pcall(ls, 2, 1, 0) == LUA_OK;
    if (!has_main) lua_pop(ls, 1);  // Remove error

#if LIB_MLUA_MOD_MLUA_THREAD
    // The mlua.thread module is available. Start a thread for the main module's
    // main function, then call mlua.thread.main.
    mlua_require(ls, "mlua.thread", true);
    if (has_main) {
        lua_getfield(ls, -1, "start");  // Start the thread
        lua_rotate(ls, -3, -1);
        lua_call(ls, 1, 1);
        luaL_getmetafield(ls, -1, "set_name");  // Set the thread name
        lua_rotate(ls, -2, 1);
        lua_pushliteral(ls, "main");
        lua_call(ls, 2, 0);
    }
    lua_getfield(ls, -1, "main");  // Run thread.main
    lua_remove(ls, -2);  // Remove thread module
    lua_call(ls, 0, 0);
#else
    // The mlua.thread module isn't available. Call the main module's main
    // function directly.
    if (has_main) lua_call(ls, 0, 0);
#endif
    return 0;
}

static void* allocate(void* ud, void* ptr, size_t old_size, size_t new_size) {
    if (new_size != 0) return realloc(ptr, new_size);
    free(ptr);
    return NULL;
}

static int on_panic(lua_State* ls) {
    char const* msg = lua_tostring(ls, -1);
    if (msg == NULL) msg = "unknown error";
    mlua_writestringerror("PANIC: %s\n", msg);
    panic(NULL);
}

static void warn_print(char const* msg, bool first, bool last) {
    if (first) mlua_writestringerror("WARNING: ", "");
    mlua_writestringerror("%s", msg);
    if (last) mlua_writestringerror("\n", "");
}

static void on_warn_cont(void* ud, char const* msg, int cont);
static void on_warn_off(void* ud, char const* msg, int cont);

static void on_warn_on(void* ud, char const* msg, int cont) {
    if (!cont && msg[0] == '@') {
        if (strcmp(msg, "@off") == 0)
            lua_setwarnf((lua_State*)ud, &on_warn_off, ud);
        return;
    }
    warn_print(msg, true, !cont);
    if (cont) lua_setwarnf((lua_State*)ud, &on_warn_cont, ud);
}

static void on_warn_cont(void* ud, char const* msg, int cont) {
    warn_print(msg, false, !cont);
    if (!cont) lua_setwarnf((lua_State*)ud, &on_warn_on, ud);
}

static void on_warn_off(void* ud, char const* msg, int cont) {
    if (cont || strcmp(msg, "@on") == 0) return;
    lua_setwarnf((lua_State*)ud, &on_warn_on, ud);
}

lua_State* mlua_new_interpreter() {
    lua_State* ls = lua_newstate(allocate, NULL);
    lua_atpanic(ls, &on_panic);
    lua_setwarnf(ls, &on_warn_off, ls);
    return ls;
}

void mlua_run_main(lua_State* ls) {
    lua_pushcfunction(ls, pmain);
    lua_rotate(ls, 1, 1);
    int err = lua_pcall(ls, 2, 0, 0);
    if (err != LUA_OK) {
        mlua_writestringerror("ERROR: %s\n", lua_tostring(ls, -1));
        lua_pop(ls, 1);
    }
}

#ifndef MLUA_MAIN_MODULE
#define MLUA_MAIN_MODULE main
#endif

#ifndef MLUA_MAIN_FUNCTION
#define MLUA_MAIN_FUNCTION main
#endif

void mlua_main_core0() {
    // Ensure that the system timer is ticking. This seems to take some time
    // after a reset.
    busy_wait_us(1);

    // Run the Lua interpreter.
    lua_State* ls = mlua_new_interpreter();
    lua_pushliteral(ls, MLUA_ESTR(MLUA_MAIN_MODULE));
    lua_pushliteral(ls, MLUA_ESTR(MLUA_MAIN_FUNCTION));
    mlua_run_main(ls);
    lua_close(ls);
}

__attribute__((weak)) int main() {
    mlua_main_core0();
    return 0;
}
