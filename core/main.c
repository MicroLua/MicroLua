// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/main.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "mlua/module.h"
#include "mlua/platform.h"
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

    // Set up stdio streams.
#if LIB_MLUA_MOD_MLUA_STDIO
    mlua_require(ls, "mlua.stdio", false);
#elif LIB_MLUA_MOD_IO
    mlua_require(ls, "io", true);
    lua_getfield(ls, -1, "stdin");
    lua_setglobal(ls, "stdin");
    lua_getfield(ls, -1, "stdout");
    lua_setglobal(ls, "stdout");
    lua_getfield(ls, -1, "stderr");
    lua_setglobal(ls, "stderr");
    lua_pop(ls, 1);
#endif

    // Enable module loading from a filesystem.
#if LIB_MLUA_MOD_MLUA_FS_LOADER
    mlua_require(ls, "mlua.fs.loader", false);
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
        lua_pushliteral(ls, "main");
        lua_call(ls, 2, 0);
    }
    lua_getfield(ls, -1, "main");  // Run thread.main
    lua_remove(ls, -2);  // Remove thread module
    lua_call(ls, 0, 1);
#else
    // The mlua.thread module isn't available. Call the main module's main
    // function directly.
    if (has_main) lua_call(ls, 0, 1);
#endif
    return 1;
}

static void* allocate(void* ud, void* ptr, size_t old_size, size_t new_size) {
    // TODO: Record per-object stats based on old_size
    if (new_size != 0) return realloc(ptr, new_size);
    free(ptr);
    return NULL;
}

static int on_panic(lua_State* ls) {
    char const* msg = lua_tostring(ls, -1);
    if (msg == NULL) msg = "unknown error";
    mlua_writestringerror("PANIC: %s\n", msg);
    mlua_platform_abort();
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

lua_State* mlua_new_interpreter(void) {
    lua_State* ls = lua_newstate(allocate, NULL);
    if (ls == NULL) return NULL;
    lua_atpanic(ls, &on_panic);
    lua_setwarnf(ls, &on_warn_off, ls);
    return ls;
}

int mlua_run_main(lua_State* ls, int args) {
    mlua_platform_setup_interpreter(ls);
    lua_pushcfunction(ls, pmain);
    lua_rotate(ls, 1, 1);

    int res = EXIT_FAILURE;
    if (lua_pcall(ls, 2 + args, 1, 0) != LUA_OK || lua_isstring(ls, -1)) {
        mlua_writestringerror("ERROR: %s\n", lua_tostring(ls, -1));
    } else if (lua_isnil(ls, -1)
               || (lua_isboolean(ls, -1) && lua_toboolean(ls, -1))) {
        res = EXIT_SUCCESS;
    } else if (lua_isinteger(ls, -1)) {
        res = lua_tointeger(ls, -1);
    }
    lua_pop(ls, 1);
    return res;
}

#ifndef MLUA_MAIN_MODULE
#define MLUA_MAIN_MODULE main
#endif

#ifndef MLUA_MAIN_FUNCTION
#define MLUA_MAIN_FUNCTION main
#endif

int mlua_main_core0(int argc, char* argv[]) {
    lua_State* ls = mlua_new_interpreter();
    if (ls == NULL) {
        mlua_writestringerror("ERROR: failed to create Lua state\n", NULL);
        return EXIT_FAILURE;
    }

    // Set _G.arg.
    if (argc > 0) {
        lua_createtable(ls, argc - 1, 1);
        for (int i = 0; i < argc; ++i) {
            lua_pushstring(ls, argv[i]);
            lua_rawseti(ls, -2, i);
        }
        lua_setglobal(ls, "arg");
    }

    // Load and run the main module.
    lua_pushliteral(ls, MLUA_ESTR(MLUA_MAIN_MODULE));
    lua_pushliteral(ls, MLUA_ESTR(MLUA_MAIN_FUNCTION));
    int res = mlua_run_main(ls, 0);
    lua_close(ls);
    return res;
}

__attribute__((weak)) int main(int argc, char* argv[]) {
    mlua_platform_setup_main(&argc, argv);
    return mlua_main_core0(argc, argv);
}
