// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/main.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#if PICO_ON_DEVICE
#include "hardware/exception.h"
#include "hardware/timer.h"
#include "pico/platform.h"
#endif

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

static int run_closure_1(lua_State* ls, int status, lua_KContext ctx) {
    return lua_gettop(ls);
}

static int run_closure(lua_State* ls) {
    for (int i = 1; !lua_isnone(ls, lua_upvalueindex(i)); ++i) {
        lua_pushvalue(ls, lua_upvalueindex(i));
    }
    lua_callk(ls, lua_gettop(ls) - 1, LUA_MULTRET, 0, &run_closure_1);
    return run_closure_1(ls, LUA_OK, 0);
}

static int pmain(lua_State* ls) {
    // Register compiled-in modules.
    mlua_register_modules(ls);
    mlua_util_init(ls);

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
    int top = lua_gettop(ls);
    if (has_main && top > 1) {  // Wrap arguments into a closure
        lua_rotate(ls, 1, 1);
        lua_pushcclosure(ls, &run_closure, top);
    }
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
    if (has_main) {
        lua_rotate(ls, 1, 1);
        lua_call(ls, lua_gettop(ls) - 1, 0);
    }
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
#if PICO_ON_DEVICE
    panic(NULL);
#else
    exit(EXIT_FAILURE);
#endif
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

#if PICO_ON_DEVICE

// This HardFault exception handler catches semihosting calls when no debugger
// is attached, and makes them silently succeed. For anything else, it does the
// same as the default handler, i.e. hang with a "bkpt 0".
//
// Note that this doesn't work if the target was reset from openocd, even if
// the latter has terminated, because the probe seems to keep watching for and
// handling hardfaults, so semihosting requests halt the core and the handler
// never gets called.
static __attribute__((naked)) void hardfault_handler(void) {
    pico_default_asm_volatile(
        "movs r0, #4\n"         // r0 = SP (from MSP or PSP)
        "mov r1, lr\n"
        "tst r0, r1\n"
        "beq 1f\n"
        "mrs r0, psp\n"
        "b 2f\n"
    "1:\n"
        "mrs r0, msp\n"
    "2:\n"
        "ldr r1, [r0, #24]\n"   // r1 = PC
        "ldrh r2, [r1]\n"       // Check if the instruction is "bkpt 0xab"
        "ldr r3, =0xbeab\n"     // (semihosting)
        "cmp r2, r3\n"
        "beq 3f\n"
        "bkpt 0x00\n"           // Not semihosting, panic
    "3:\n"
        "adds r1, #2\n"         // Semihosting, skip the bkpt instruction
        "str r1, [r0, #24]\n"
        "movs r1, #0\n"         // Set the result of the semihosting call to 0
        "str r1, [r0, #0]\n"
        "bx lr\n"               // Return to the caller
    );
}

void isr_hardfault(void);

#endif  // PICO_ON_DEVICE

lua_State* mlua_new_interpreter(void) {
    lua_State* ls = lua_newstate(allocate, NULL);
    lua_atpanic(ls, &on_panic);
    lua_setwarnf(ls, &on_warn_off, ls);
    return ls;
}

int main_msg_handler(lua_State* ls) {
    // Check if the error starts with a location ("module:123: ").
    char const* msg = lua_tostring(ls, 1);
    if (msg == NULL) return 1;
    char const* sep1 = strchr(msg, ':');
    if (sep1 == NULL || sep1 == msg) return 1;
    char const* sep2 = sep1 + 1;
    for (;; ++sep2) {
        char c = *sep2;
        if (c == ':') break;
        if (!isdigit(c)) return 1;
    }
    if (sep2 == sep1 + 1 || *(sep2 + 1) != ' ') return 1;

    // Check if the location refers to a loaded module.
    lua_getfield(ls, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    lua_pushlstring(ls, msg, sep1 - msg);
    bool found = lua_gettable(ls, -2) != LUA_TNIL;
    lua_pop(ls, 2);
    if (!found) return 1;

    // Append a traceback to the error.
    luaL_traceback(ls, ls, msg, 1);
    return 1;
}

int mlua_run_main(lua_State* ls, int args) {
#if PICO_ON_DEVICE
    // Set the HardFault exception handler if none was set before.
    exception_handler_t handler =
        exception_get_vtable_handler(HARDFAULT_EXCEPTION);
    if (handler == &isr_hardfault) {
        exception_set_exclusive_handler(HARDFAULT_EXCEPTION,
                                        &hardfault_handler);
    }
#endif

    lua_pushcfunction(ls, main_msg_handler);
    lua_pushcfunction(ls, pmain);
    lua_rotate(ls, 1, 2);

    int res = EXIT_FAILURE;
    if (lua_pcall(ls, 2 + args, 1, 1) != LUA_OK || lua_isstring(ls, -1)) {
        mlua_writestringerror("ERROR: %s\n", lua_tostring(ls, -1));
    } else if (lua_isnil(ls, -1)) {
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

#if PICO_ON_DEVICE

__attribute__((weak)) int main() {
    // Ensure that the system timer is ticking. This seems to take some time
    // after a reset.
    busy_wait_us(1);

    return mlua_main_core0(0, NULL);
}

#else

__attribute__((weak)) int main(int argc, char* argv[]) {
    return mlua_main_core0(argc, argv);
}

#endif
