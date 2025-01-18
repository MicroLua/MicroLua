// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>
#include <stdlib.h>

#include "hardware/structs/sio.h"
#include "hardware/sync.h"
#include "pico.h"
#include "pico/multicore.h"
#include "pico/time.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

#if LIB_MLUA_MOD_MLUA_THREAD

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
        mlua_event_set(&st->event);
    }
}

static int mod_enable_irq(lua_State* ls) {
    uint core = get_core_num();
    if (!mlua_event_enable_irq(ls, &fifo_state[core].event,
                              SIO_IRQ_PROC0 + core, &handle_sio_irq, 1, -1)) {
        return luaL_error(ls, "SIO%d: IRQ already enabled", core);
    }
    return 0;
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

static int mod_push_blocking_1(lua_State* ls, int status, lua_KContext ctx);

static int mod_push_blocking(lua_State* ls) {
    if (!mlua_thread_blocking(ls)) return mod_push_blocking_1(ls, LUA_OK, 0);
    multicore_fifo_push_blocking(luaL_checkinteger(ls, 1));
    return 0;
}

#if LIB_MLUA_MOD_MLUA_THREAD

static int mod_push_blocking_1(lua_State* ls, int status, lua_KContext ctx) {
    // Busy loop, as there is no interrupt for RDY.
    if (multicore_fifo_wready()) {
        sio_hw->fifo_wr = luaL_checkinteger(ls, 1);
        __sev();  // In case the other end is doing blocking reads
        return 0;
    }
    return mlua_thread_yield(ls, 0, &mod_push_blocking_1, 0);
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

static int mod_push_timeout_us_1(lua_State* ls, int status, lua_KContext ctx);

static int mod_push_timeout_us(lua_State* ls) {
    uint64_t timeout = mlua_check_int64(ls, 2);
    if (!mlua_thread_blocking(ls)) {
        lua_settop(ls, 1);
        mlua_push_deadline(ls, timeout);
        return mod_push_timeout_us_1(ls, LUA_OK, 0);
    }
    uint32_t data = luaL_checkinteger(ls, 1);
    lua_pushboolean(ls, multicore_fifo_push_timeout_us(data, timeout));
    return 1;
}

#if LIB_MLUA_MOD_MLUA_THREAD

static int mod_push_timeout_us_1(lua_State* ls, int status, lua_KContext ctx) {
    // Busy loop, as there is no interrupt for RDY.
    if (multicore_fifo_wready()) {
        sio_hw->fifo_wr = luaL_checkinteger(ls, 1);
        lua_pushboolean(ls, true);
        return 1;
    } else if (mlua_time_reached(ls, 2)) {
        lua_pushboolean(ls, false);
        return 1;
    }
    return mlua_thread_yield(ls, 0, &mod_push_timeout_us_1, 0);
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

static int pop_loop(lua_State* ls, bool timeout) {
    if (multicore_fifo_rvalid()) {
        lua_pushinteger(ls, sio_hw->fifo_rd);
        return 1;
    }
    if (timeout) return 0;
    irq_set_enabled(SIO_IRQ_PROC0 + get_core_num(), true);
    return -1;
}

static int mod_pop_blocking(lua_State* ls) {
    MLuaEvent* event = &fifo_state[get_core_num()].event;
    if (mlua_event_can_wait(ls, event, 0)) {
        return mlua_event_wait(ls, event, 0, &pop_loop, 0);
    }
    lua_pushinteger(ls, multicore_fifo_pop_blocking());
    return 1;
}

static int mod_pop_timeout_us(lua_State* ls) {
    uint64_t timeout = mlua_check_int64(ls, 1);
    MLuaEvent* event = &fifo_state[get_core_num()].event;
    if (mlua_event_can_wait(ls, event, 0)) {
        lua_settop(ls, 0);
        mlua_push_deadline(ls, timeout);
        return mlua_event_wait(ls, event, 0, &pop_loop, 1);
    }

    // BUG(pico-sdk): We don't use multicore_fifo_pop_timeout_us() here, because
    // of <https://github.com/raspberrypi/pico-sdk/issues/1142>.
    absolute_time_t deadline = make_timeout_time_us(timeout);
    while (!multicore_fifo_rvalid()) {
        if (best_effort_wfe_or_timeout(deadline)) return false;
    }
    lua_pushinteger(ls, sio_hw->fifo_rd);
    return 1;
}

static int mod_clear_irq(lua_State* ls) {
#if LIB_MLUA_MOD_MLUA_THREAD
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

static int mod_get_status(lua_State* ls) {
#if LIB_MLUA_MOD_MLUA_THREAD
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

MLUA_FUNC_R0(mod_, multicore_fifo_, rvalid, lua_pushboolean)
MLUA_FUNC_R0(mod_, multicore_fifo_, wready, lua_pushboolean)
MLUA_FUNC_V0(mod_, multicore_fifo_, drain)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(ROE, integer, SIO_FIFO_ST_ROE_BITS),
    MLUA_SYM_V(WOF, integer, SIO_FIFO_ST_WOF_BITS),
    MLUA_SYM_V(RDY, integer, SIO_FIFO_ST_RDY_BITS),
    MLUA_SYM_V(VLD, integer, SIO_FIFO_ST_VLD_BITS),

    MLUA_SYM_F(rvalid, mod_),
    MLUA_SYM_F(wready, mod_),
    MLUA_SYM_F(push_blocking, mod_),
    MLUA_SYM_F(push_timeout_us, mod_),
    MLUA_SYM_F(pop_blocking, mod_),
    MLUA_SYM_F(pop_timeout_us, mod_),
    MLUA_SYM_F(drain, mod_),
    MLUA_SYM_F(clear_irq, mod_),
    MLUA_SYM_F(get_status, mod_),
    MLUA_SYM_F_THREAD(enable_irq, mod_),
};

MLUA_OPEN_MODULE(pico.multicore.fifo) {
    mlua_thread_require(ls);
    mlua_require(ls, "mlua.int64", false);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
