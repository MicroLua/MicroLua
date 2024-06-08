// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/main.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "mlua/module.h"
#include "mlua/platform.h"
#if LIB_MLUA_MOD_MLUA_THREAD
#include "mlua/thread.h"
#endif
#include "mlua/util.h"

// The name of the module containing the main function.
#ifndef MLUA_MAIN_MODULE
#define MLUA_MAIN_MODULE main
#endif

// The name of the main function.
#ifndef MLUA_MAIN_FUNCTION
#define MLUA_MAIN_FUNCTION main
#endif

// Log errors raised by the main function to stderr.
#ifndef MLUA_MAIN_LOG_ERRORS
#define MLUA_MAIN_LOG_ERRORS 1
#endif

// Shut down the thread scheduler when the main function terminates.
#ifndef MLUA_MAIN_SHUTDOWN
#define MLUA_MAIN_SHUTDOWN 1
#endif

// TODO: Allow passing arguments to main() and returning results => multicore

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

    // Get the main function.
#if LIB_MLUA_MOD_MLUA_THREAD
    mlua_require(ls, "mlua.thread", true);
    lua_getfield(ls, -1, "start");
#endif
    lua_rotate(ls, 1, -1);
    lua_call(ls, 0, 1);

#if LIB_MLUA_MOD_MLUA_THREAD
    // Start a thread for the main function.
    lua_pushliteral(ls, "main");
    lua_call(ls, 2, 0);

    // Call mlua.thread.main instead of the main function.
    lua_getfield(ls, -1, "main");
    lua_remove(ls, -2);  // Remove thread module
#endif
    return lua_call(ls, 0, 1), 1;
}

static void* allocate(void* ud, void* ptr, size_t old_size, size_t new_size) {
    // TODO: Record per-object stats based on old_size
    if (new_size != 0) {
        void* res = realloc(ptr, new_size);
#if MLUA_ALLOC_STATS
        if (res == NULL) return NULL;
        MLuaGlobal* g = ud;
        ++g->alloc_count;
        g->alloc_size += new_size;
        if (ptr != NULL) new_size -= old_size;
        g->alloc_used += new_size;
        if (g->alloc_used > g->alloc_peak) g->alloc_peak = g->alloc_used;
#endif
        return res;
    }
    free(ptr);
#if MLUA_ALLOC_STATS
    ((MLuaGlobal*)ud)->alloc_used -= old_size;
#endif
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
        if (strcmp(msg, "@off") == 0) lua_setwarnf(ud, &on_warn_off, ud);
        return;
    }
    warn_print(msg, true, !cont);
    if (cont) lua_setwarnf(ud, &on_warn_cont, ud);
}

static void on_warn_cont(void* ud, char const* msg, int cont) {
    warn_print(msg, false, !cont);
    if (!cont) lua_setwarnf(ud, &on_warn_on, ud);
}

static void on_warn_off(void* ud, char const* msg, int cont) {
    if (cont || strcmp(msg, "@on") == 0) return;
    lua_setwarnf(ud, &on_warn_on, ud);
}

lua_State* mlua_new_interpreter(void) {
    MLuaGlobal* g = realloc(NULL, sizeof(MLuaGlobal));
    if (g == NULL) return NULL;
    memset(g, 0, sizeof(*g));
    lua_State* ls = lua_newstate(allocate, g);
    if (ls == NULL) {
        free(g);
        return NULL;
    }
    lua_atpanic(ls, &on_panic);
    lua_setwarnf(ls, &on_warn_off, ls);
    memset(lua_getextraspace(ls), 0, LUA_EXTRASPACE);
    return ls;
}

void mlua_close_interpreter(lua_State* ls) {
    void* ud;
    lua_getallocf(ls, &ud);
    lua_close(ls);
#if MLUA_ALLOC_STATS
    if (((MLuaGlobal*)ud)->alloc_used != 0) {
        mlua_writestringerror("WARNING: interpreter memory leak\n", "");
    }
#endif
    free(ud);
}

static int log_error(lua_State* ls) {
    char const* msg = lua_tostring(ls, 1);
    if (msg == NULL) msg = luaL_tolstring(ls, 1, NULL);
    luaL_traceback(ls, ls, msg, 1);
    mlua_writestringerror("ERROR: %s\n", lua_tostring(ls, -1));
    return lua_settop(ls, 1), 1;
}

int mlua_run_main(lua_State* ls, int nargs, int nres, int msgh) {
    mlua_platform_setup_interpreter(ls);
    lua_pushcfunction(ls, pmain);
    lua_rotate(ls, -(nargs + 2), 1);
    return lua_pcall(ls, 1 + nargs, nres, msgh);
}

#if LIB_MLUA_MOD_MLUA_THREAD

static int main_done(lua_State* ls) {
    if (!lua_isyieldable(ls)) return 0;
    mlua_thread_meta(ls, "shutdown");
    lua_pushvalue(ls, lua_upvalueindex(1));
    return lua_callk(ls, 1, 0, 0, &mlua_cont_return_ctx), 0;
}

static int shutdown_on_exit_2(lua_State* ls, int status, lua_KContext ctx);

static int shutdown_on_exit(lua_State* ls) {
    lua_pushnil(ls);
    lua_pushcclosure(ls, &main_done, 1);
    lua_pushvalue(ls, lua_upvalueindex(1));
    lua_rotate(ls, 1, 2);
    lua_toclose(ls, 1);
    int status = lua_pcallk(ls, lua_gettop(ls) - 2, 1, 0, 0,
                            &shutdown_on_exit_2);
    return shutdown_on_exit_2(ls, status, 0);
}

static int shutdown_on_exit_2(lua_State* ls, int status, lua_KContext ctx) {
    lua_setupvalue(ls, 1, 1);  // First result or error
    return 0;
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

static int find_main(lua_State* ls) {
    mlua_require(ls, MLUA_ESTR(MLUA_MAIN_MODULE), true);
    lua_getfield(ls, -1, MLUA_ESTR(MLUA_MAIN_FUNCTION));
#if MLUA_MAIN_LOG_ERRORS
    lua_getglobal(ls, "log_errors");
    lua_rotate(ls, -2, 1);
    lua_call(ls, 1, 1);
#endif
#if LIB_MLUA_MOD_MLUA_THREAD && MLUA_MAIN_SHUTDOWN
    lua_pushcclosure(ls, &shutdown_on_exit, 1);
#endif
    return 1;
}

int mlua_main_core0(int argc, char* argv[]) {
    lua_State* ls = mlua_new_interpreter();
    if (ls == NULL) {
        mlua_writestringerror("ERROR: failed to create Lua state\n", NULL);
        return EXIT_FAILURE;
    }

#if MLUA_MAIN_LOG_ERRORS
    lua_pushcfunction(ls, &log_error);
#endif

    // Set _G.arg.
    // TODO: Pass as a single argument to main()
    if (argc > 0) {
        lua_createtable(ls, argc - 1, 1);
        for (int i = 0; i < argc; ++i) {
            lua_pushstring(ls, argv[i]);
            lua_rawseti(ls, -2, i);
        }
        lua_setglobal(ls, "arg");
    }

    // Load the main module and run the main function.
    lua_pushcfunction(ls, &find_main);
    int res = EXIT_FAILURE;
    if (mlua_run_main(ls, 0, 1, MLUA_MAIN_LOG_ERRORS ? 1 : 0) != LUA_OK) {
#if !MLUA_MAIN_LOG_ERRORS
        mlua_writestringerror("ERROR: %s\n", lua_tostring(ls, -1));
#endif
    } else if (lua_isstring(ls, -1)) {
        mlua_writestringerror("ERROR: %s\n", lua_tostring(ls, -1));
    } else if (lua_isnil(ls, -1)
               || (lua_isboolean(ls, -1) && lua_toboolean(ls, -1))) {
        res = EXIT_SUCCESS;
    } else if (lua_isinteger(ls, -1)) {
        res = lua_tointeger(ls, -1);
    }
    lua_pop(ls, MLUA_MAIN_LOG_ERRORS ? 2 : 1);
    mlua_close_interpreter(ls);
    return res;
}

__attribute__((weak)) int main(int argc, char* argv[]) {
    mlua_platform_setup_main(&argc, argv);
    return mlua_main_core0(argc, argv);
}
