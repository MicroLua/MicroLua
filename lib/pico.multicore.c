#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/time.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/int64.h"
#include "mlua/main.h"
#include "mlua/util.h"

// TODO: Move FIFO stuff to pico.multicore.fifo

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

#if LIB_MLUA_MOD_MLUA_EVENT

typedef struct FifoState {
    MLuaEvent event;
    uint8_t status;
} FifoState;

static FifoState fifo_state[NUM_CORES];

static void __time_critical_func(handle_sio_irq)(void) {
    uint core = get_core_num();
    FifoState* st = &fifo_state[core];
    uint32_t status = multicore_fifo_get_status();
    multicore_fifo_clear_irq();
    st->status |= status & (SIO_FIFO_ST_ROE_BITS | SIO_FIFO_ST_WOF_BITS);
    if (status & SIO_FIFO_ST_VLD_BITS) {
        irq_set_enabled(SIO_IRQ_PROC0 + core, false);
        mlua_event_set(st->event);
    }
}

static int mod_fifo_enable_irq(lua_State* ls) {
    uint core = get_core_num();
    char const* err = mlua_event_enable_irq(
        ls, &fifo_state[core].event, SIO_IRQ_PROC0 + core,
        &handle_sio_irq, 1, -1);
    if (err != NULL) return luaL_error(ls, "SIO[%d]: %s", core, err);
    return 0;
}

#endif  // LIB_MLUA_MOD_MLUA_EVENT

static int mod_fifo_push_blocking_1(lua_State* ls, int status,
                                    lua_KContext ctx);

static int mod_fifo_push_blocking(lua_State* ls) {
    if (mlua_yield_enabled()) return mod_fifo_push_blocking_1(ls, 0, 0);
    multicore_fifo_push_blocking(luaL_checkinteger(ls, 1));
    return 0;
}

#if LIB_MLUA_MOD_MLUA_EVENT

static int mod_fifo_push_blocking_1(lua_State* ls, int status,
                                    lua_KContext ctx) {
    // Busy loop, as there is no interrupt for RDY.
    if (multicore_fifo_wready()) {
        multicore_fifo_push_blocking(luaL_checkinteger(ls, 1));
        return 0;
    }
    return mlua_event_yield(ls, &mod_fifo_push_blocking_1, 0, 0);
}

#endif  // LIB_MLUA_MOD_MLUA_EVENT

static int mod_fifo_push_timeout_us_1(lua_State* ls, int status,
                                      lua_KContext ctx);

static int mod_fifo_push_timeout_us(lua_State* ls) {
    uint64_t timeout = mlua_check_int64(ls, 2);
    if (mlua_yield_enabled()) {
        mlua_push_int64(ls, to_us_since_boot(make_timeout_time_us(timeout)));
        lua_replace(ls, 2);
        return mod_fifo_push_timeout_us_1(ls, 0, 0);
    }
    uint32_t data = luaL_checkinteger(ls, 1);
    lua_pushboolean(ls, multicore_fifo_push_timeout_us(data, timeout));
    return 1;
}

#if LIB_MLUA_MOD_MLUA_EVENT

static int mod_fifo_push_timeout_us_1(lua_State* ls, int status,
                                      lua_KContext ctx) {
    // Busy loop, as there is no interrupt for RDY.
    if (multicore_fifo_wready()) {
        multicore_fifo_push_blocking(luaL_checkinteger(ls, 1));
        lua_pushboolean(ls, true);
        return 1;
    } else if (time_reached(from_us_since_boot(mlua_check_int64(ls, 2)))) {
        lua_pushboolean(ls, false);
        return 1;
    }
    return mlua_event_yield(ls, &mod_fifo_push_timeout_us_1, 0, 0);
}

#endif  // LIB_MLUA_MOD_MLUA_EVENT

static int try_fifo_pop(lua_State* ls, bool timeout) {
    if (timeout) return 0;
    if (multicore_fifo_rvalid()) {
        lua_pushinteger(ls, multicore_fifo_pop_blocking());
        return 1;
    }
    irq_set_enabled(SIO_IRQ_PROC0 + get_core_num(), true);
    return -1;
}

static int mod_fifo_pop_blocking(lua_State* ls) {
    if (mlua_yield_enabled()) {
        return mlua_event_wait(ls, fifo_state[get_core_num()].event,
                               &try_fifo_pop, 0);
    }
    lua_pushinteger(ls, multicore_fifo_pop_blocking());
    return 1;
}

static int mod_fifo_pop_timeout_us(lua_State* ls) {
    absolute_time_t deadline = make_timeout_time_us(mlua_check_int64(ls, 1));
    if (mlua_yield_enabled()) {
        mlua_push_int64(ls, to_us_since_boot(deadline));
        lua_replace(ls, 1);
        return mlua_event_wait(ls, fifo_state[get_core_num()].event,
                               &try_fifo_pop, 1);
    }

    // We don't use multicore_fifo_pop_timeout_us() here, because of
    // <https://github.com/raspberrypi/pico-sdk/issues/1142>.
    while (!multicore_fifo_rvalid()) {
        if (best_effort_wfe_or_timeout(deadline)) return false;
    }
    lua_pushinteger(ls, multicore_fifo_pop_blocking());
    return 1;
}

static int mod_fifo_clear_irq(lua_State* ls) {
#if LIB_MLUA_MOD_MLUA_EVENT
    FifoState* st = &fifo_state[get_core_num()];
    uint32_t save = save_and_disable_interrupts();
    multicore_fifo_clear_irq();
    st->status = 0;
    restore_interrupts(save);
#else
    multicore_fifo_clear_irq();
#endif
    return 0;
}

static int mod_fifo_get_status(lua_State* ls) {
#if LIB_MLUA_MOD_MLUA_EVENT
    FifoState* st = &fifo_state[get_core_num()];
    uint32_t save = save_and_disable_interrupts();
    uint32_t status = multicore_fifo_get_status() | st->status;
    restore_interrupts(save);
#else
    uint32_t status = multicore_fifo_get_status();
#endif
    lua_pushinteger(ls, status);
    return 1;
}

MLUA_FUNC_1_0(mod_, multicore_, fifo_rvalid, lua_pushboolean)
MLUA_FUNC_1_0(mod_, multicore_, fifo_wready, lua_pushboolean)
MLUA_FUNC_0_0(mod_, multicore_, fifo_drain)

static MLuaSym const module_syms[] = {
    MLUA_SYM_V(FIFO_ROE, integer, SIO_FIFO_ST_ROE_BITS),
    MLUA_SYM_V(FIFO_WOF, integer, SIO_FIFO_ST_WOF_BITS),
    MLUA_SYM_V(FIFO_RDY, integer, SIO_FIFO_ST_RDY_BITS),
    MLUA_SYM_V(FIFO_VLD, integer, SIO_FIFO_ST_VLD_BITS),

    MLUA_SYM_F(reset_core1, mod_),
    MLUA_SYM_F(launch_core1, mod_),
    MLUA_SYM_F(wait_shutdown, mod_),
    MLUA_SYM_F(fifo_rvalid, mod_),
    MLUA_SYM_F(fifo_wready, mod_),
    MLUA_SYM_F(fifo_push_blocking, mod_),
    MLUA_SYM_F(fifo_push_timeout_us, mod_),
    MLUA_SYM_F(fifo_pop_blocking, mod_),
    MLUA_SYM_F(fifo_pop_timeout_us, mod_),
    MLUA_SYM_F(fifo_drain, mod_),
    MLUA_SYM_F(fifo_clear_irq, mod_),
    MLUA_SYM_F(fifo_get_status, mod_),
#if LIB_MLUA_MOD_MLUA_EVENT
    MLUA_SYM_F(fifo_enable_irq, mod_),
#endif
};

static __attribute__((constructor)) void init(void) {
    for (uint core = 0; core < NUM_CORES; ++core) {
        if (core != 0) {
            core_state[core - 1].shutdown_event = MLUA_EVENT_UNSET;
            core_state[core - 1].stopped_event = MLUA_EVENT_UNSET;
        }
#if LIB_MLUA_MOD_MLUA_EVENT
        fifo_state[core].event = MLUA_EVENT_UNSET;
#endif
    }
}

int luaopen_pico_multicore(lua_State* ls) {
    mlua_event_require(ls);
    mlua_require(ls, "mlua.int64", false);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
