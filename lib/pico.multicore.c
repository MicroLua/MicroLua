#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/structs/sio.h"
#include "pico/multicore.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/main.h"
#include "mlua/util.h"

typedef struct CoreState {
    lua_State* ls;
    MLuaEvent shutdown_event;
    MLuaEvent stopped_event;
    bool shutdown;
} CoreState;

static CoreState core_state[NUM_CORES - 1];

static void launch_core(void) {
    CoreState* st = &core_state[get_core_num() - 1];
    lua_State* ls = st->ls;
    mlua_run_main(ls);
    mlua_event_unclaim(ls, &st->shutdown_event);
    lua_close(ls);
    uint32_t save = mlua_event_lock();
    mlua_event_set_nolock(st->stopped_event);
    mlua_event_unlock(save);
}

static int mod_launch_core1(lua_State* ls) {
    uint const core = 1;
    if (get_core_num() == core) {
        return luaL_error(ls, "cannot launch core %d from itself", core);
    }
    size_t len;
    char const* module = luaL_checklstring(ls, 1, &len);
    char const* fn = luaL_optstring(ls, 2, "main");

    // Set up the shutdown request event.
    CoreState* st = &core_state[core - 1];
    char const* err = mlua_event_claim_core(&st->shutdown_event, core);
    if (err == mlua_event_err_already_claimed) {
        return luaL_error(ls, "core %d is already running an interpreter",
                          core);
    } else if (err != NULL) {
        return luaL_error(ls, "multicore: %s", err);
    }

    // Create a new interpreter.
    st->ls = mlua_new_interpreter();
    st->shutdown = false;

    // Launch the interpreter in the other core.
    lua_pushlstring(st->ls, module, len);
    lua_pushstring(st->ls, fn);
    multicore_launch_core1(&launch_core);
    return 0;
}

static int try_stopped(lua_State* ls, bool timeout) {
    CoreState* st = (CoreState*)lua_touserdata(ls, -1);
    uint32_t save = mlua_event_lock();
    bool stopped = st->shutdown_event == MLUA_EVENT_UNSET;
    mlua_event_unlock(save);
    if (!stopped) return -1;
    mlua_event_unclaim(ls, &st->stopped_event);
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
    uint32_t save = mlua_event_lock();
    bool running = st->shutdown_event != MLUA_EVENT_UNSET;
    if (running) {
        st->shutdown = true;
        mlua_event_set_nolock(st->shutdown_event);
    }
    mlua_event_unlock(save);
    if (!running) {
        multicore_reset_core1();
        return 0;
    }

    // Wait for the interpreter to terminate.
    char const* err = mlua_event_claim(&st->stopped_event);
    if (err != NULL) return luaL_error(ls, "multicore: %s", err);
    lua_pushlightuserdata(ls, st);
    return mlua_event_wait(ls, st->stopped_event, &try_stopped, 0);
}

static int try_shutdown(lua_State* ls, bool timeout) {
    CoreState* st = &core_state[get_core_num() - 1];
    uint32_t save = mlua_event_lock();
    bool shutdown = st->shutdown;
    mlua_event_unlock(save);
    return shutdown ? 0 : -1;
}

static int mod_wait_shutdown(lua_State* ls) {
    uint core = get_core_num();
    if (core == 0) return luaL_error(ls, "cannot wait for shutdown on core 0");
    return mlua_event_wait(ls, core_state[core - 1].shutdown_event,
                           &try_shutdown, 0);
}

static MLuaSym const module_syms[] = {
    MLUA_SYM_F(reset_core1, mod_),
    MLUA_SYM_F(launch_core1, mod_),
    MLUA_SYM_F(wait_shutdown, mod_),
};

static __attribute__((constructor)) void init(void) {
    for (uint core = 1; core < NUM_CORES; ++core) {
        core_state[core - 1].shutdown_event = MLUA_EVENT_UNSET;
        core_state[core - 1].stopped_event = MLUA_EVENT_UNSET;
    }
}

int luaopen_pico_multicore(lua_State* ls) {
    mlua_event_require(ls);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
