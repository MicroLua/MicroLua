// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/address_mapped.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/sync.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

static inline PIO get_pio_instance(uint num) {
    return num == 0 ? pio0 : pio1;
}

static char const Config_name[] = "hardware.pio.Config";

static inline pio_sm_config* check_Config(lua_State* ls, int arg) {
    return (pio_sm_config*)luaL_checkudata(ls, arg, Config_name);
}

static int Config_clkdiv(lua_State* ls) {
    pio_sm_config const* cfg = check_Config(ls, 1);
    return lua_pushinteger(ls, cfg->clkdiv), 1;
}

static int Config_execctrl(lua_State* ls) {
    pio_sm_config const* cfg = check_Config(ls, 1);
    return lua_pushinteger(ls, cfg->execctrl), 1;
}

static int Config_shiftctrl(lua_State* ls) {
    pio_sm_config const* cfg = check_Config(ls, 1);
    return lua_pushinteger(ls, cfg->shiftctrl), 1;
}

static int Config_pinctrl(lua_State* ls) {
    pio_sm_config const* cfg = check_Config(ls, 1);
    return lua_pushinteger(ls, cfg->pinctrl), 1;
}

MLUA_FUNC_S3(Config_, sm_config_, set_out_pins, check_Config, luaL_checkinteger,
             luaL_checkinteger)
MLUA_FUNC_S3(Config_, sm_config_, set_set_pins, check_Config, luaL_checkinteger,
             luaL_checkinteger)
MLUA_FUNC_S2(Config_, sm_config_, set_in_pins, check_Config, luaL_checkinteger)
MLUA_FUNC_S2(Config_, sm_config_, set_sideset_pins, check_Config,
             luaL_checkinteger)
MLUA_FUNC_S4(Config_, sm_config_, set_sideset, check_Config, luaL_checkinteger,
             mlua_to_cbool, mlua_to_cbool)
MLUA_FUNC_S(Config_, sm_config_, set_clkdiv_int_frac, check_Config(ls, 1),
            luaL_checkinteger(ls, 2), luaL_optinteger(ls, 3, 0));
MLUA_FUNC_S2(Config_, sm_config_, set_clkdiv, check_Config, luaL_checknumber)
MLUA_FUNC_S3(Config_, sm_config_, set_wrap, check_Config, luaL_checkinteger,
             luaL_checkinteger)
MLUA_FUNC_S2(Config_, sm_config_, set_jmp_pin, check_Config, luaL_checkinteger)
MLUA_FUNC_S4(Config_, sm_config_, set_in_shift, check_Config, mlua_to_cbool,
             mlua_to_cbool, luaL_checkinteger)
MLUA_FUNC_S4(Config_, sm_config_, set_out_shift, check_Config, mlua_to_cbool,
             mlua_to_cbool, luaL_checkinteger)
MLUA_FUNC_S2(Config_, sm_config_, set_fifo_join, check_Config,
             luaL_checkinteger)
MLUA_FUNC_S4(Config_, sm_config_, set_out_special, check_Config, mlua_to_cbool,
             mlua_to_cbool, luaL_checkinteger)
MLUA_FUNC_S3(Config_, sm_config_, set_mov_status, check_Config,
             luaL_checkinteger, luaL_checkinteger)

MLUA_SYMBOLS(Config_syms) = {
    MLUA_SYM_F(clkdiv, Config_),
    MLUA_SYM_F(execctrl, Config_),
    MLUA_SYM_F(shiftctrl, Config_),
    MLUA_SYM_F(pinctrl, Config_),
    MLUA_SYM_F(set_out_pins, Config_),
    MLUA_SYM_F(set_set_pins, Config_),
    MLUA_SYM_F(set_in_pins, Config_),
    MLUA_SYM_F(set_sideset_pins, Config_),
    MLUA_SYM_F(set_sideset, Config_),
    MLUA_SYM_F(set_clkdiv_int_frac, Config_),
    MLUA_SYM_F(set_clkdiv, Config_),
    MLUA_SYM_F(set_wrap, Config_),
    MLUA_SYM_F(set_jmp_pin, Config_),
    MLUA_SYM_F(set_in_shift, Config_),
    MLUA_SYM_F(set_out_shift, Config_),
    MLUA_SYM_F(set_fifo_join, Config_),
    MLUA_SYM_F(set_out_special, Config_),
    MLUA_SYM_F(set_mov_status, Config_),
};

static char const SM_name[] = "hardware.pio.SM";

typedef struct SM {
    PIO pio;
    uint8_t sm;
} SM;

static SM* new_SM(lua_State* ls) {
    SM* v = lua_newuserdatauv(ls, sizeof(SM), 0);
    luaL_getmetatable(ls, SM_name);
    lua_setmetatable(ls, -2);
    return v;
}

static inline SM* check_SM(lua_State* ls, int arg) {
    return (SM*)luaL_checkudata(ls, arg, SM_name);
}

static inline SM* to_SM(lua_State* ls, int arg) {
    return (SM*)lua_touserdata(ls, arg);
}

#define SM_FUNC_V(n, ...)  \
static int SM_ ## n(lua_State* ls) { \
    SM* sm = check_SM(ls, 1); \
    pio_sm_ ## n(sm->pio, sm->sm, ## __VA_ARGS__); \
    return 0; \
}
#define SM_FUNC_R(n, ret, ...)  \
static int SM_ ## n(lua_State* ls) { \
    SM* sm = check_SM(ls, 1); \
    return ret(ls, pio_sm_ ## n(sm->pio, sm->sm, ## __VA_ARGS__)), 1; \
}

#define SM_FUNC_V0(n) SM_FUNC_V(n)
#define SM_FUNC_V1(n, a1) SM_FUNC_V(n, a1(ls, 2))
#define SM_FUNC_V2(n, a1, a2) SM_FUNC_V(n, a1(ls, 2), a2(ls, 3))
#define SM_FUNC_V3(n, a1, a2, a3) SM_FUNC_V(n, a1(ls, 2), a2(ls, 3), a3(ls, 4))
#define SM_FUNC_R0(n, ret) SM_FUNC_R(n, ret)
#define SM_FUNC_R1(n, ret, a1) SM_FUNC_R(n, ret, a1(ls, 2))

static int SM_index(lua_State* ls) {
    SM* sm = check_SM(ls, 1);
    return lua_pushinteger(ls, sm->sm), 1;
}

static int SM_get_dreq(lua_State* ls) {
    SM* sm = check_SM(ls, 1);
    return lua_pushinteger(ls, pio_get_dreq(sm->pio, sm->sm,
                                            mlua_to_cbool(ls, 2))), 1;
}

#if LIB_MLUA_MOD_MLUA_THREAD

typedef struct PIOIntRegs {
    io_rw_32 inte;
    io_rw_32 intf;
    io_rw_32 ints;
} PIOIntRegs;

#define INT_REGS(pio) (((PIOIntRegs*)&pio->inte0) + get_core_num())

typedef struct SMState {
    MLuaEvent rx_event;
    MLuaEvent tx_event;
} SMState;

typedef struct PIOState {
    SMState sm[NUM_PIO_STATE_MACHINES];
    MLuaEvent irq_event[NUM_CORES];
    uint32_t mask[NUM_CORES];
    uint8_t pending[NUM_CORES];
} PIOState;

static PIOState pio_state[NUM_PIOS];

#define FIFO_IRQ_MASK \
    ((((1u << NUM_PIO_STATE_MACHINES) - 1) << PIO_INTR_SM0_RXNEMPTY_LSB) \
     | (((1u << NUM_PIO_STATE_MACHINES) - 1) << PIO_INTR_SM0_TXNFULL_LSB))
#define SM_IRQ_MASK \
    (((1u << NUM_PIO_STATE_MACHINES) - 1) << PIO_INTR_SM0_LSB)

static void __time_critical_func(handle_pio_irq)(void) {
    uint num = (__get_current_exception() - VTABLE_FIRST_IRQ - PIO0_IRQ_0) / 2;
    PIO inst = get_pio_instance(num);
    PIOIntRegs* ir = INT_REGS(inst);
    uint32_t ints = ir->ints;
    hw_clear_bits(&ir->inte, ints);  // Mask IRQs that are firing
    PIOState* state = &pio_state[num];
    for (uint i = 0; i < NUM_PIO_STATE_MACHINES; ++i) {
        SMState* sm = &state->sm[i];
        if ((ints & (PIO_INTR_SM0_RXNEMPTY_BITS << i)) != 0) {
            mlua_event_set(&sm->rx_event);
        }
        if ((ints & (PIO_INTR_SM0_TXNFULL_BITS << i)) != 0) {
            mlua_event_set(&sm->tx_event);
        }
    }
    uint8_t pending = (ints & SM_IRQ_MASK) >> PIO_INTR_SM0_LSB;
    if (pending != 0) {
        uint core = get_core_num();
        state->pending[core] |= pending;
        mlua_event_set(&state->irq_event[core]);
    }
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

static int put_loop(lua_State* ls, bool timeout) {
    SM* sm = to_SM(ls, 1);
    if (pio_sm_is_tx_fifo_full(sm->pio, sm->sm)) {
        hw_set_bits(&INT_REGS(sm->pio)->inte,
                    PIO_INTR_SM0_TXNFULL_BITS << sm->sm);
        return -1;
    }
    pio_sm_put(sm->pio, sm->sm, lua_tointeger(ls, 2));
    return 0;
}

static int SM_put_blocking(lua_State* ls) {
    SM* sm = check_SM(ls, 1);
    uint32_t data = luaL_checkinteger(ls, 2);
    PIOState* state = &pio_state[pio_get_index(sm->pio)];
    MLuaEvent* ev = &state->sm[sm->sm].tx_event;
    if (((state->mask[get_core_num()]
            & (PIO_INTR_SM0_TXNFULL_BITS << sm->sm)) != 0)
            && mlua_event_can_wait(ls, ev)) {
        lua_settop(ls, 2);
        return mlua_event_loop(ls, ev, &put_loop, 0);
    }
    pio_sm_put_blocking(sm->pio, sm->sm, data);
    return 0;
}

static int get_loop(lua_State* ls, bool timeout) {
    SM* sm = to_SM(ls, 1);
    if (pio_sm_is_rx_fifo_empty(sm->pio, sm->sm)) {
        hw_set_bits(&INT_REGS(sm->pio)->inte,
                    PIO_INTR_SM0_RXNEMPTY_BITS << sm->sm);
        return -1;
    }
    return lua_pushinteger(ls, pio_sm_get(sm->pio, sm->sm)), 1;
}

static int SM_get_blocking(lua_State* ls) {
    SM* sm = check_SM(ls, 1);
    PIOState* state = &pio_state[pio_get_index(sm->pio)];
    MLuaEvent* ev = &state->sm[sm->sm].rx_event;
    if (((state->mask[get_core_num()]
            & (PIO_INTR_SM0_RXNEMPTY_BITS << sm->sm)) != 0)
            && mlua_event_can_wait(ls, ev)) {
        lua_settop(ls, 1);
        return mlua_event_loop(ls, ev, &get_loop, 0);
    }
    return lua_pushinteger(ls, pio_sm_get_blocking(sm->pio, sm->sm)), 1;
}

SM_FUNC_V1(set_config, check_Config)
SM_FUNC_V2(init, luaL_checkinteger, check_Config)
SM_FUNC_V1(set_enabled, mlua_to_cbool)
SM_FUNC_V0(restart)
SM_FUNC_V0(clkdiv_restart)
SM_FUNC_R0(get_pc, lua_pushinteger)
SM_FUNC_V1(exec, luaL_checkinteger)
SM_FUNC_R0(is_exec_stalled, lua_pushboolean)
SM_FUNC_V1(exec_wait_blocking, luaL_checkinteger)
SM_FUNC_V2(set_wrap, luaL_checkinteger, luaL_checkinteger)
SM_FUNC_V2(set_out_pins, luaL_checkinteger, luaL_checkinteger)
SM_FUNC_V2(set_set_pins, luaL_checkinteger, luaL_checkinteger)
SM_FUNC_V1(set_in_pins, luaL_checkinteger)
SM_FUNC_V1(set_sideset_pins, luaL_checkinteger)
SM_FUNC_V1(put, luaL_checkinteger)
SM_FUNC_R0(get, lua_pushinteger)
SM_FUNC_R0(is_rx_fifo_full, lua_pushboolean)
SM_FUNC_R0(is_rx_fifo_empty, lua_pushboolean)
SM_FUNC_R0(get_rx_fifo_level, lua_pushinteger)
SM_FUNC_R0(is_tx_fifo_full, lua_pushboolean)
SM_FUNC_R0(is_tx_fifo_empty, lua_pushboolean)
SM_FUNC_R0(get_tx_fifo_level, lua_pushinteger)
SM_FUNC_V0(drain_tx_fifo)
SM_FUNC_V2(set_clkdiv_int_frac, luaL_checkinteger, luaL_checkinteger)
SM_FUNC_V1(set_clkdiv, luaL_checknumber)
SM_FUNC_V0(clear_fifos)
SM_FUNC_V1(set_pins, luaL_checkinteger)
SM_FUNC_V2(set_pins_with_mask, luaL_checkinteger, luaL_checkinteger)
SM_FUNC_V2(set_pindirs_with_mask, luaL_checkinteger, luaL_checkinteger)
SM_FUNC_V3(set_consecutive_pindirs, luaL_checkinteger, luaL_checkinteger,
           mlua_to_cbool)
SM_FUNC_V0(claim)
SM_FUNC_V0(unclaim)
SM_FUNC_R0(is_claimed, lua_pushboolean)

MLUA_SYMBOLS(SM_syms) = {
    MLUA_SYM_F(index, SM_),
    MLUA_SYM_F(set_config, SM_),
    MLUA_SYM_F(get_dreq, SM_),
    MLUA_SYM_F(init, SM_),
    MLUA_SYM_F(set_enabled, SM_),
    MLUA_SYM_F(restart, SM_),
    MLUA_SYM_F(clkdiv_restart, SM_),
    MLUA_SYM_F(get_pc, SM_),
    MLUA_SYM_F(exec, SM_),
    MLUA_SYM_F(is_exec_stalled, SM_),
    MLUA_SYM_F(exec_wait_blocking, SM_),
    MLUA_SYM_F(set_wrap, SM_),
    MLUA_SYM_F(set_out_pins, SM_),
    MLUA_SYM_F(set_set_pins, SM_),
    MLUA_SYM_F(set_in_pins, SM_),
    MLUA_SYM_F(set_sideset_pins, SM_),
    MLUA_SYM_F(put, SM_),
    MLUA_SYM_F(get, SM_),
    MLUA_SYM_F(is_rx_fifo_full, SM_),
    MLUA_SYM_F(is_rx_fifo_empty, SM_),
    MLUA_SYM_F(get_rx_fifo_level, SM_),
    MLUA_SYM_F(is_tx_fifo_full, SM_),
    MLUA_SYM_F(is_tx_fifo_empty, SM_),
    MLUA_SYM_F(get_tx_fifo_level, SM_),
    MLUA_SYM_F(put_blocking, SM_),
    MLUA_SYM_F(get_blocking, SM_),
    MLUA_SYM_F(drain_tx_fifo, SM_),
    MLUA_SYM_F(set_clkdiv_int_frac, SM_),
    MLUA_SYM_F(set_clkdiv, SM_),
    MLUA_SYM_F(clear_fifos, SM_),
    MLUA_SYM_F(set_pins, SM_),
    MLUA_SYM_F(set_pins_with_mask, SM_),
    MLUA_SYM_F(set_pindirs_with_mask, SM_),
    MLUA_SYM_F(set_consecutive_pindirs, SM_),
    MLUA_SYM_F(claim, SM_),
    MLUA_SYM_F(unclaim, SM_),
    MLUA_SYM_F(is_claimed, SM_),
};

static char const PIO_name[] = "hardware.pio.PIO";

static PIO* new_PIO(lua_State* ls) {
    PIO* v = lua_newuserdatauv(ls, sizeof(PIO), NUM_PIO_STATE_MACHINES);
    luaL_getmetatable(ls, PIO_name);
    lua_setmetatable(ls, -2);
    return v;
}

static inline PIO check_PIO(lua_State* ls, int arg) {
    return *((PIO*)luaL_checkudata(ls, arg, PIO_name));
}

static inline PIO to_PIO(lua_State* ls, int arg) {
    return *((PIO*)lua_touserdata(ls, arg));
}

static int PIO_sm(lua_State* ls) {
    PIO inst = check_PIO(ls, 1);
    lua_Unsigned num = luaL_checkinteger(ls, 2);
    luaL_argcheck(ls, num < NUM_PIO_STATE_MACHINES, 2, "invalid state machine");
    if (lua_getiuservalue(ls, 1, num + 1) == LUA_TNIL) {
        lua_pop(ls, 1);
        SM* sm = new_SM(ls);
        sm->pio = inst;
        sm->sm = num;
        lua_pushvalue(ls, -1);
        lua_setiuservalue(ls, 1, num + 1);
    }
    return 1;
}

static int PIO_regs(lua_State* ls) {
    lua_pushinteger(ls, (uintptr_t)check_PIO(ls, 1));
    return 1;
}

static void to_program(lua_State* ls, pio_program_t* prog) {
    prog->length = lua_rawlen(ls, 2);
    if (prog->length > PIO_INSTRUCTION_COUNT) {
        luaL_error(ls, "program too large");
        return;
    }
    if (lua_getfield(ls, 2, "origin") != LUA_TNIL) {
        prog->origin = luaL_checkinteger(ls, -1);
    }
    lua_pop(ls, 1);
}

static int PIO_can_add_program(lua_State* ls) {
    PIO inst = check_PIO(ls, 1);
    luaL_checktype(ls, 2, LUA_TTABLE);
    pio_program_t prog;
    to_program(ls, &prog);
    return lua_pushboolean(ls, pio_can_add_program(inst, &prog)), 1;
}

static int PIO_can_add_program_at_offset(lua_State* ls) {
    PIO inst = check_PIO(ls, 1);
    luaL_checktype(ls, 2, LUA_TTABLE);
    uint off = luaL_checkinteger(ls, 3);
    pio_program_t prog;
    to_program(ls, &prog);
    lua_pushboolean(ls, pio_can_add_program_at_offset(inst, &prog, off));
    return 1;
}

static void copy_instructions(lua_State* ls, uint8_t len, uint16_t* instr) {
    for (uint8_t i = 0; i < len; ++i) {
        lua_rawgeti(ls, 2, i + 1);
        instr[i] = luaL_checkinteger(ls, -1);
        lua_pop(ls, 1);
    }
}

static int PIO_add_program(lua_State* ls) {
    PIO inst = check_PIO(ls, 1);
    luaL_checktype(ls, 2, LUA_TTABLE);
    pio_program_t prog;
    to_program(ls, &prog);
    uint16_t instr[prog.length];
    prog.instructions = instr;
    copy_instructions(ls, prog.length, instr);
    return lua_pushinteger(ls, pio_add_program(inst, &prog)), 1;
}

static int PIO_add_program_at_offset(lua_State* ls) {
    PIO inst = check_PIO(ls, 1);
    luaL_checktype(ls, 2, LUA_TTABLE);
    uint off = luaL_checkinteger(ls, 3);
    pio_program_t prog;
    to_program(ls, &prog);
    uint16_t instr[prog.length];
    prog.instructions = instr;
    copy_instructions(ls, prog.length, instr);
    pio_add_program_at_offset(inst, &prog, off);
    return 0;
}

static int PIO_remove_program(lua_State* ls) {
    PIO inst = check_PIO(ls, 1);
    pio_program_t prog;
    if (lua_type(ls, 2) == LUA_TTABLE) {
        prog.length = luaL_len(ls, 2);
    } else if (lua_isinteger(ls, 2)) {
        prog.length = lua_tointeger(ls, 2);
    } else {
        return luaL_typeerror(ls, 2, "table or integer");
    }
    uint off = luaL_checkinteger(ls, 3);
    pio_remove_program(inst, &prog, off);
    return 0;
}

#if LIB_MLUA_MOD_MLUA_THREAD

static int handle_sm_irq_event_1(lua_State* ls, int status, lua_KContext ctx);

static int handle_sm_irq_event(lua_State* ls) {
    PIO inst = to_PIO(ls, lua_upvalueindex(1));
    uint core = get_core_num();
    PIOState* state = &pio_state[pio_get_index(inst)];
    uint32_t save = save_and_disable_interrupts();
    uint8_t pending = state->pending[core];
    state->pending[core] = 0;
    restore_interrupts(save);
    pending &= (state->mask[core] & SM_IRQ_MASK) >> PIO_INTR_SM0_LSB;
    if (pending != 0) {  // Call the callback
        lua_pushvalue(ls, lua_upvalueindex(2));  // handler
        lua_pushinteger(ls, pending);  // pending
        lua_callk(ls, 1, 0, pending, &handle_sm_irq_event_1);
    }
    return handle_sm_irq_event_1(ls, LUA_OK, pending);
}

static int handle_sm_irq_event_1(lua_State* ls, int status, lua_KContext ctx) {
    PIO inst = to_PIO(ls, lua_upvalueindex(1));
    PIOState* state = &pio_state[pio_get_index(inst)];
    uint32_t pending = ctx;
    hw_set_bits(&INT_REGS(inst)->inte,
                (pending << PIO_INTR_SM0_LSB) & state->mask[get_core_num()]
                & SM_IRQ_MASK);
    return 0;
}

static int sm_irq_handler_done(lua_State* ls) {
    PIO inst = to_PIO(ls, lua_upvalueindex(1));
    PIOState* state = &pio_state[pio_get_index(inst)];
    mlua_event_disable(ls, &state->irq_event[get_core_num()]);
    return 0;
}

static int PIO_set_irq_callback(lua_State* ls) {
    PIO inst = check_PIO(ls, 1);
    PIOState* state = &pio_state[pio_get_index(inst)];
    uint core = get_core_num();
    MLuaEvent* ev = &state->irq_event[core];
    if (lua_isnoneornil(ls, 2)) {  // Nil callback, kill the handler thread
        mlua_event_stop_handler(ls, ev);
        return 0;
    }

    // Set the callback.
    if (!mlua_event_enable(ls, ev)) {
        return luaL_error(ls, "PIO%d: SM IRQ callback already set",
                          pio_get_index(inst));
    }
    uint32_t save = save_and_disable_interrupts();
    state->pending[core] = 0;
    restore_interrupts(save);
    hw_set_bits(&INT_REGS(inst)->inte, state->mask[core] & SM_IRQ_MASK);

    // Start the event handler thread.
    lua_pushvalue(ls, 1);  // PIO
    lua_pushvalue(ls, 2);  // handler
    lua_pushcclosure(ls, &handle_sm_irq_event, 2);
    lua_pushvalue(ls, 1);  // PIO
    lua_pushcclosure(ls, &sm_irq_handler_done, 1);
    return mlua_event_handle(ls, ev, &mlua_cont_return_ctx, 1);
}

static void disable_events(lua_State* ls, PIOState* state, uint32_t mask) {
    for (int i = NUM_PIO_STATE_MACHINES - 1; i >= 0; --i) {
        SMState* sm = &state->sm[i];
        if ((mask & (PIO_INTR_SM0_TXNFULL_BITS << i)) != 0) {
            mlua_event_disable(ls, &sm->tx_event);
        }
        if ((mask & (PIO_INTR_SM0_RXNEMPTY_BITS << i)) != 0) {
            mlua_event_disable(ls, &sm->rx_event);
        }
    }
}

static void enable_events(lua_State* ls, PIOState* state, uint32_t mask) {
    uint32_t done = 0;
    for (uint i = 0; i < NUM_PIO_STATE_MACHINES; ++i) {
        SMState* sm = &state->sm[i];
        uint32_t b = PIO_INTR_SM0_RXNEMPTY_BITS << i;
        if ((mask & b) != 0) {
            if (!mlua_event_enable(ls, &sm->rx_event)) {
                disable_events(ls, state, done);
                luaL_error(ls, "PIO%d: SM%d Rx IRQ already enabled",
                           state - pio_state, i);
                return;
            }
            done |= b;
        }
        b = PIO_INTR_SM0_TXNFULL_BITS << i;
        if ((mask & b) != 0) {
            if (!mlua_event_enable(ls, &sm->tx_event)) {
                disable_events(ls, state, done);
                luaL_error(ls, "PIO%d: SM%d Tx IRQ already enabled",
                           state - pio_state, i);
                return;
            }
            done |= b;
        }
    }
}

static int PIO_enable_irq(lua_State* ls) {
    PIO inst = check_PIO(ls, 1);
    uint32_t mask = luaL_checkinteger(ls, 2) & (FIFO_IRQ_MASK | SM_IRQ_MASK);
    uint num = pio_get_index(inst);
    uint irq = PIO0_IRQ_0 + get_core_num() + 2 * num;
    PIOState* state = &pio_state[num];
    lua_Integer priority = -1;
    if (!mlua_event_enable_irq_arg(ls, 3, &priority)) {  // Disable IRQs
        uint core = get_core_num();
        mask &= state->mask[core];
        if (mask == 0) return 0;
        state->mask[core] &= ~mask;
        hw_clear_bits(&INT_REGS(inst)->inte, mask & SM_IRQ_MASK);
        if (state->mask[core] == 0) {
            irq_set_enabled(irq, false);
            irq_remove_handler(irq, &handle_pio_irq);
        }
        disable_events(ls, state, mask);
        return 0;
    }

    // Enable IRQs.
    uint core = get_core_num();
    mask &= ~state->mask[core];
    if (mask == 0) return 0;
    enable_events(ls, state, mask);
    state->mask[core] |= mask;
    if (state->mask[core] == mask) {
        mlua_event_set_irq_handler(irq, &handle_pio_irq, priority);
        irq_set_enabled(irq, true);
    }
    hw_set_bits(&INT_REGS(inst)->inte, mask & SM_IRQ_MASK);
    return 0;
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

static int PIO_interrupt_get_mask(lua_State* ls) {
    return lua_pushinteger(ls, check_PIO(ls, 1)->irq), 1;
}

static int PIO_interrupt_clear_mask(lua_State* ls) {
    check_PIO(ls, 1)->irq = luaL_checkinteger(ls, 2);
    return 0;
}

MLUA_FUNC_R1(PIO_, pio_, get_index, lua_pushinteger, check_PIO)
MLUA_FUNC_V2(PIO_, pio_, gpio_init, check_PIO, luaL_checkinteger)
MLUA_FUNC_V1(PIO_, pio_, clear_instruction_memory, check_PIO)
MLUA_FUNC_V3(PIO_, pio_, set_sm_mask_enabled, check_PIO, luaL_checkinteger,
             mlua_to_cbool)
MLUA_FUNC_V2(PIO_, pio_, restart_sm_mask, check_PIO, luaL_checkinteger)
MLUA_FUNC_V2(PIO_, pio_, clkdiv_restart_sm_mask, check_PIO, luaL_checkinteger)
MLUA_FUNC_V2(PIO_, pio_, enable_sm_mask_in_sync, check_PIO, luaL_checkinteger)
MLUA_FUNC_R2(PIO_, pio_, interrupt_get, lua_pushboolean, check_PIO,
             luaL_checkinteger)
MLUA_FUNC_V2(PIO_, pio_, interrupt_clear, check_PIO, luaL_checkinteger)
MLUA_FUNC_V2(PIO_, pio_, claim_sm_mask, check_PIO, luaL_checkinteger)
MLUA_FUNC_R(PIO_, pio_, claim_unused_sm, lua_pushinteger, check_PIO(ls, 1),
             mlua_opt_cbool(ls, 2, true))

MLUA_SYMBOLS(PIO_syms) = {
    MLUA_SYM_F(sm, PIO_),
    MLUA_SYM_F(get_index, PIO_),
    MLUA_SYM_F(regs, PIO_),
    MLUA_SYM_F(gpio_init, PIO_),
    MLUA_SYM_F(can_add_program, PIO_),
    MLUA_SYM_F(can_add_program_at_offset, PIO_),
    MLUA_SYM_F(add_program, PIO_),
    MLUA_SYM_F(add_program_at_offset, PIO_),
    MLUA_SYM_F(remove_program, PIO_),
    MLUA_SYM_F(clear_instruction_memory, PIO_),
    MLUA_SYM_F(set_sm_mask_enabled, PIO_),
    MLUA_SYM_F(restart_sm_mask, PIO_),
    MLUA_SYM_F(clkdiv_restart_sm_mask, PIO_),
    MLUA_SYM_F(enable_sm_mask_in_sync, PIO_),
    // set_irq0_source_enabled: Not useful in Lua
    // set_irq1_source_enabled: Not useful in Lua
    // set_irq0_source_mask_enabled: Not useful in Lua
    // set_irq1_source_mask_enabled: Not useful in Lua
    // set_irqn_source_enabled: Not useful in Lua
    // set_irqn_source_mask_enabled: Not useful in Lua
    MLUA_SYM_F(interrupt_get, PIO_),
    MLUA_SYM_F(interrupt_get_mask, PIO_),
    MLUA_SYM_F(interrupt_clear, PIO_),
    MLUA_SYM_F(interrupt_clear_mask, PIO_),
    MLUA_SYM_F(claim_sm_mask, PIO_),
    MLUA_SYM_F(claim_unused_sm, PIO_),
#if LIB_MLUA_MOD_MLUA_THREAD
    MLUA_SYM_F(set_irq_callback, PIO_),
#else
    MLUA_SYM_V(set_irq_callback, boolean, false),
#endif
#if LIB_MLUA_MOD_MLUA_THREAD
    MLUA_SYM_F(enable_irq, PIO_),
#else
    MLUA_SYM_V(enable_irq, boolean, false),
#endif
};

static int mod_get_default_sm_config(lua_State* ls) {
    pio_sm_config* cfg = lua_newuserdatauv(ls, sizeof(pio_sm_config), 0);
    luaL_getmetatable(ls, Config_name);
    lua_setmetatable(ls, -2);
    *cfg = pio_get_default_sm_config();
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(NUM, integer, NUM_PIOS),
    MLUA_SYM_V(NUM_STATE_MACHINES, integer, NUM_PIO_STATE_MACHINES),
    MLUA_SYM_V(FIFO_JOIN_NONE, integer, PIO_FIFO_JOIN_NONE),
    MLUA_SYM_V(FIFO_JOIN_TX, integer, PIO_FIFO_JOIN_TX),
    MLUA_SYM_V(FIFO_JOIN_RX, integer, PIO_FIFO_JOIN_RX),
    MLUA_SYM_V(STATUS_TX_LESSTHAN, integer, STATUS_TX_LESSTHAN),
    MLUA_SYM_V(STATUS_RX_LESSTHAN, integer, STATUS_RX_LESSTHAN),
    MLUA_SYM_V(pis_interrupt0, integer, pis_interrupt0),
    MLUA_SYM_V(pis_interrupt1, integer, pis_interrupt1),
    MLUA_SYM_V(pis_interrupt2, integer, pis_interrupt2),
    MLUA_SYM_V(pis_interrupt3, integer, pis_interrupt3),
    MLUA_SYM_V(pis_sm0_tx_fifo_not_full, integer, pis_sm0_tx_fifo_not_full),
    MLUA_SYM_V(pis_sm1_tx_fifo_not_full, integer, pis_sm1_tx_fifo_not_full),
    MLUA_SYM_V(pis_sm2_tx_fifo_not_full, integer, pis_sm2_tx_fifo_not_full),
    MLUA_SYM_V(pis_sm3_tx_fifo_not_full, integer, pis_sm3_tx_fifo_not_full),
    MLUA_SYM_V(pis_sm0_rx_fifo_not_empty, integer, pis_sm0_rx_fifo_not_empty),
    MLUA_SYM_V(pis_sm1_rx_fifo_not_empty, integer, pis_sm1_rx_fifo_not_empty),
    MLUA_SYM_V(pis_sm2_rx_fifo_not_empty, integer, pis_sm2_rx_fifo_not_empty),
    MLUA_SYM_V(pis_sm3_rx_fifo_not_empty, integer, pis_sm3_rx_fifo_not_empty),

    MLUA_SYM_F(get_default_sm_config, mod_),
};

MLUA_OPEN_MODULE(hardware.pio) {
    mlua_thread_require(ls);

    // Create the module.
    mlua_new_module(ls, NUM_PIOS, module_syms);
    int mod_index = lua_gettop(ls);

    // Create the Config class.
    mlua_new_class(ls, Config_name, Config_syms, true);
    lua_pop(ls, 1);

    // Create the SM class.
    mlua_new_class(ls, SM_name, SM_syms, true);
    lua_pop(ls, 1);

    // Create the PIO class.
    mlua_new_class(ls, PIO_name, PIO_syms, true);
    lua_pop(ls, 1);

    // Create PIO instances.
    for (uint i = 0; i < NUM_PIOS; ++i) {
        *new_PIO(ls) = get_pio_instance(i);
        lua_seti(ls, mod_index, i);
    }
    return 1;
}
