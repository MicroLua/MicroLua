// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/structs/sio.h"
#include "pico.h"
#include "pico/multicore.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/main.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

// TODO: Allow passing arguments to and returning results from main() on core 1

typedef struct CoreState {
    lua_State* ls;
    MLuaEvent shutdown_event;
    MLuaEvent stopped_event;
    bool shutdown;
} CoreState;

static CoreState core_state[NUM_CORES - 1];

static int find_main(lua_State* ls) {
    lua_getglobal(ls, "require");
    lua_pushvalue(ls, lua_upvalueindex(1));
    lua_call(ls, 1, 1);
    lua_pushvalue(ls, lua_upvalueindex(2));
    lua_gettable(ls, -2);
    return 1;
}

static void launch_core(void) {
    CoreState* st = &core_state[get_core_num() - 1];
    lua_State* ls = st->ls;
    mlua_run_main(ls, 0, 0, 0);
    // TODO: The event should be disabled after closing the lua_State,
    //       otherwise the core could be reset before the state can be closed.
    mlua_event_disable(ls, &st->shutdown_event);
    mlua_close_interpreter(ls);
    mlua_event_set(&st->stopped_event);
}

static int mod_launch_core1(lua_State* ls) {
    uint const core = 1;
    if (get_core_num() == core) {
        return luaL_error(ls, "cannot launch core %d from itself", core);
    }
    size_t mlen, flen;
    char const* module = luaL_checklstring(ls, 1, &mlen);
    char const* fn = luaL_optlstring(ls, 2, "main", &flen);

    // Create a new interpreter.
    lua_State* ls1 = mlua_new_interpreter();
    if (ls1 == NULL) return luaL_error(ls, "interpreter creation failed");

    // Set up the shutdown request event.
    CoreState* st = &core_state[core - 1];
    if (!mlua_event_enable(ls1, &st->shutdown_event)) {
        mlua_close_interpreter(ls1);
        return luaL_error(ls, "core %d is already running an interpreter",
                          core);
    }
    st->ls = ls1;
    st->shutdown = false;

    // Launch the interpreter in the other core.
    lua_pushlstring(ls1, module, mlen);
    lua_pushlstring(ls1, fn, flen);
    lua_pushcclosure(ls1, &find_main, 2);
    multicore_launch_core1(&launch_core);
    return 0;
}

static int stopped_loop(lua_State* ls, bool timeout) {
    CoreState* st = (CoreState*)lua_touserdata(ls, -1);
    if (mlua_event_enabled(&st->shutdown_event)) return -1;
    mlua_event_disable(ls, &st->stopped_event);
    multicore_reset_core1();
    return 0;
}

static int mod_reset_core1(lua_State* ls) {
    uint const core = 1;
    if (get_core_num() == core) {
        return luaL_error(ls, "cannot reset core %d from itself", core);
    }
    CoreState* st = &core_state[core - 1];

    // Trigger the shutdown event if the other core is running an interpreter.
    mlua_event_lock();
    bool running = mlua_event_enabled_nolock(&st->shutdown_event);
    if (running) {
        st->shutdown = true;
        mlua_event_set_nolock(&st->shutdown_event);
    }
    mlua_event_unlock();
    if (!running) {
        multicore_reset_core1();
        return 0;
    }

    // Wait for the interpreter to terminate.
    // TODO: This is racy. Core 1 could terminate before stopped_event is
    //       enabled. Then the wait below will get stuck.
    if (!mlua_event_enable(ls, &st->stopped_event)) {
        return luaL_error(ls, "multicore: stopped event already enabled");
    }
    lua_pushlightuserdata(ls, st);
    return mlua_event_wait(ls, &st->stopped_event, 0, &stopped_loop, 0);
}

static int handle_shutdown_event(lua_State* ls) {
    CoreState* st = &core_state[get_core_num() - 1];
    mlua_event_lock();
    bool shutdown = st->shutdown;
    mlua_event_unlock();
    if (!shutdown) return 0;

    // Call the callback
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &st->shutdown);
    return mlua_callk(ls, 0, 0, mlua_cont_return, 0);
}

static int shutdown_handler_done(lua_State* ls) {
    lua_pushnil(ls);
    lua_rawsetp(ls, LUA_REGISTRYINDEX,
                &core_state[get_core_num() - 1].shutdown);
    return 0;
}

static int mod_set_shutdown_handler(lua_State* ls) {
    uint core = get_core_num();
    if (core == 0) {
        return luaL_error(ls, "cannot set a shutdown handler on core %d", core);
    }
    CoreState* st = &core_state[core - 1];
    if (lua_isnone(ls, 1)) {  // No argument, use Thread.shutdown by default
        if (mlua_thread_meta(ls, "shutdown") == LUA_TNIL) {
            return luaL_error(ls, "handler required");
        }
    } else if (lua_isnil(ls, 1)) {  // Nil handler, kill the handler thread
        mlua_event_stop_handler(ls, &st->shutdown_event);
        return 0;
    }

    // Set the handler.
    lua_settop(ls, 1);  // handler
    lua_rawsetp(ls, LUA_REGISTRYINDEX, &st->shutdown);

    // Start the event handler thread if it isn't already running.
    mlua_event_push_handler_thread(ls, &st->shutdown_event);
    if (!lua_isnil(ls, -1)) return 1;
    lua_pop(ls, 1);
    lua_pushcfunction(ls, &handle_shutdown_event);
    lua_pushcfunction(ls, &shutdown_handler_done);
    return mlua_event_handle(ls, &st->shutdown_event, &mlua_cont_return, 1);
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(reset_core1, mod_),
    MLUA_SYM_F(launch_core1, mod_),
    // multicore_launch_core1_with_stack: Not useful in Lua
    // multicore_launch_core1_raw: Not useful in Lua
    // TODO: MLUA_SYM_F(wait_core1, mod_),
    MLUA_SYM_F(set_shutdown_handler, mod_),
};

MLUA_OPEN_MODULE(pico.multicore) {
    mlua_thread_require(ls);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
