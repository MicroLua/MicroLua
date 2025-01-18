// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/address_mapped.h"
#include "hardware/dma.h"
#include "pico.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

static uint check_channel(lua_State* ls, int arg) {
    lua_Unsigned num = luaL_checkinteger(ls, arg);
    luaL_argcheck(ls, num < NUM_DMA_CHANNELS, arg, "invalid DMA channel");
    return num;
}

static uint check_timer(lua_State* ls, int arg) {
    lua_Unsigned num = luaL_checkinteger(ls, arg);
    luaL_argcheck(ls, num < NUM_DMA_TIMERS, arg, "invalid DMA timer");
    return num;
}

static void const* check_read_addr(lua_State* ls, int arg) {
    if (lua_isnil(ls, arg)) return NULL;
    MLuaBuffer buf;
    if (mlua_get_ro_buffer(ls, arg, &buf) && buf.vt == NULL) return buf.ptr;
    luaL_typeerror(ls, arg, "nil, string or raw buffer");
    return NULL;
}

static void* check_write_addr(lua_State* ls, int arg) {
    if (lua_isnil(ls, arg)) return NULL;
    MLuaBuffer buf;
    if (mlua_get_buffer(ls, arg, &buf) && buf.vt == NULL) return buf.ptr;
    luaL_typeerror(ls, arg, "nil or raw buffer");
    return NULL;
}

static char const Config_name[] = "hardware.dma.Config";

static dma_channel_config* new_Config(lua_State* ls) {
    dma_channel_config* cfg = lua_newuserdatauv(
        ls, sizeof(dma_channel_config), 0);
    luaL_getmetatable(ls, Config_name);
    lua_setmetatable(ls, -2);
    return cfg;
}

static inline dma_channel_config* check_Config(lua_State* ls, int arg) {
    return luaL_checkudata(ls, arg, Config_name);
}

static int Config_ctrl(lua_State* ls) {
    dma_channel_config const* cfg = check_Config(ls, 1);
    return lua_pushinteger(ls, channel_config_get_ctrl_value(cfg)), 1;
}

MLUA_FUNC_S2(Config_, channel_config_, set_read_increment, check_Config,
             mlua_to_cbool)
MLUA_FUNC_S2(Config_, channel_config_, set_write_increment, check_Config,
             mlua_to_cbool)
MLUA_FUNC_S2(Config_, channel_config_, set_dreq, check_Config,
             luaL_checkinteger)
MLUA_FUNC_S2(Config_, channel_config_, set_chain_to, check_Config,
             check_channel)
MLUA_FUNC_S2(Config_, channel_config_, set_transfer_data_size, check_Config,
             luaL_checkinteger)
MLUA_FUNC_S3(Config_, channel_config_, set_ring, check_Config, mlua_to_cbool,
             luaL_checkinteger)
MLUA_FUNC_S2(Config_, channel_config_, set_bswap, check_Config, mlua_to_cbool)
MLUA_FUNC_S2(Config_, channel_config_, set_irq_quiet, check_Config,
             mlua_to_cbool)
MLUA_FUNC_S2(Config_, channel_config_, set_high_priority, check_Config,
             mlua_to_cbool)
MLUA_FUNC_S2(Config_, channel_config_, set_enable, check_Config, mlua_to_cbool)
MLUA_FUNC_S2(Config_, channel_config_, set_sniff_enable, check_Config,
             mlua_to_cbool)

MLUA_SYMBOLS(Config_syms) = {
    MLUA_SYM_F(ctrl, Config_),
    MLUA_SYM_F(set_read_increment, Config_),
    MLUA_SYM_F(set_write_increment, Config_),
    MLUA_SYM_F(set_dreq, Config_),
    MLUA_SYM_F(set_chain_to, Config_),
    MLUA_SYM_F(set_transfer_data_size, Config_),
    MLUA_SYM_F(set_ring, Config_),
    MLUA_SYM_F(set_bswap, Config_),
    MLUA_SYM_F(set_irq_quiet, Config_),
    MLUA_SYM_F(set_high_priority, Config_),
    MLUA_SYM_F(set_enable, Config_),
    MLUA_SYM_F(set_sniff_enable, Config_),
};

#if LIB_MLUA_MOD_MLUA_THREAD

typedef struct DMAIntRegs {
    io_rw_32 inte;
    io_rw_32 intf;
    io_rw_32 ints;
} DMAIntRegs;

#define INT_REGS(core) ((DMAIntRegs*)&dma_hw->inte0 + core)

typedef struct DMAState {
    MLuaEvent events[NUM_DMA_CHANNELS];
    uint16_t pending[NUM_CORES];
} DMAState;

static DMAState dma_state;

static void __time_critical_func(handle_dma_irq)(void) {
    uint core = get_core_num();
    DMAIntRegs* ir = INT_REGS(core);
    uint32_t pending = ir->ints;
    if (pending == 0) return;
    ir->ints = pending;  // Clear pending IRQs
    dma_state.pending[core] |= pending;
    while (pending != 0) {
        uint ch = MLUA_CTZ(pending);
        pending &= ~(1u << ch);
        mlua_event_set(&dma_state.events[ch]);
    }
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

static int mod_channel_get_default_config(lua_State* ls) {
    *new_Config(ls) = dma_channel_get_default_config(check_channel(ls, 1));
    return 1;
}

static int mod_get_channel_config(lua_State* ls) {
    *new_Config(ls) = dma_channel_get_default_config(check_channel(ls, 1));
    return 1;
}

static int mod_regs(lua_State* ls) {
    lua_pushlightuserdata(ls, lua_isnoneornil(ls, 1) ? (void*)dma_hw
                              : (void*)&dma_hw->ch[check_channel(ls, 1)]);
    return 1;
}

static int mod_channel_get_irq_status(lua_State* ls) {
    uint16_t mask = 1u << check_channel(ls, 1);
    bool pending = (dma_hw->intr & mask) != 0;
#if LIB_MLUA_MOD_MLUA_THREAD
    if (!pending) {
        uint32_t save = save_and_disable_interrupts();
        pending = (dma_state.pending[get_core_num()] & mask) != 0;
        restore_interrupts(save);
    }
#endif
    return lua_pushboolean(ls, pending), 1;
}

static int mod_channel_acknowledge_irq(lua_State* ls) {
    uint16_t mask = 1u << check_channel(ls, 1);
#if LIB_MLUA_MOD_MLUA_THREAD
    uint32_t save = save_and_disable_interrupts();
    dma_hw->intr = mask;
    dma_state.pending[get_core_num()] &= ~mask;
    restore_interrupts(save);
#else
    dma_hw->intr = mask;
#endif
    return 0;
}

static int wait_irq_loop(lua_State* ls, bool timeout) {
    uint16_t mask = lua_tointeger(ls, 1);
    uint16_t* pending = &dma_state.pending[get_core_num()];
    uint32_t save = save_and_disable_interrupts();
    uint16_t p = *pending;
    *pending &= ~mask;
    restore_interrupts(save);
    p &= mask;
    if (p == 0) return -1;
    return lua_pushinteger(ls, p), 1;
}

static int mod_wait_irq(lua_State* ls) {
    uint16_t mask = luaL_checkinteger(ls, 1) & MLUA_MASK(NUM_DMA_CHANNELS);
    MLuaEvent const* evs = dma_state.events;
    uint m = mlua_event_multi(&evs, mask);
    if (mlua_event_can_wait(ls, evs, m)) {
        return mlua_event_wait(ls, evs, m, &wait_irq_loop, 0);
    }

    uint16_t poll_mask = mask;
#if LIB_MLUA_MOD_MLUA_THREAD
    uint core = get_core_num();
    uint16_t* pending = &dma_state.pending[core];
    uint16_t irq_mask = INT_REGS(core)->inte & mask;
    poll_mask &= ~irq_mask;
#endif
    for (;;) {
        uint16_t p = dma_hw->intr & poll_mask;
        if (p != 0) dma_hw->intr = p;
#if LIB_MLUA_MOD_MLUA_THREAD
        if (irq_mask != 0) {
            uint32_t save = save_and_disable_interrupts();
            p |= *pending & irq_mask;
            *pending &= ~irq_mask;
            restore_interrupts(save);
        }
#endif
        if (p != 0) return lua_pushinteger(ls, p), 1;
        tight_loop_contents();
    }
}

#if LIB_MLUA_MOD_MLUA_THREAD

static void disable_events(lua_State* ls, uint32_t mask) {
    while (mask != 0) {
        uint ch = MLUA_CTZ(mask);
        mask &= ~(1u << ch);
        mlua_event_disable(ls, &dma_state.events[ch]);
    }
}

static void enable_events(lua_State* ls, uint32_t mask) {
    uint32_t done = 0;
    while (mask != 0) {
        uint ch = MLUA_CTZ(mask);
        uint32_t bm = 1u << ch;
        mask &= ~bm;
        if (!mlua_event_enable(ls, &dma_state.events[ch])) {
            disable_events(ls, done);
            luaL_error(ls, "DMA: channel %d IRQ already enabled", ch);
            return;
        }
        done |= bm;
    }
}

static int mod_enable_irq(lua_State* ls) {
    uint32_t mask = luaL_checkinteger(ls, 1) & MLUA_MASK(NUM_DMA_CHANNELS);
    uint core = get_core_num();
    uint irq = DMA_IRQ_0 + core;
    DMAIntRegs* ir = INT_REGS(core);
    lua_Integer priority = -1;
    if (!mlua_event_enable_irq_arg(ls, 2, &priority)) {  // Disable IRQs
        mask &= ir->inte;
        if (mask == 0) return 0;
        hw_clear_bits(&ir->inte, mask);
        if (ir->inte == 0) {
            irq_set_enabled(irq, false);
            irq_remove_handler(irq, &handle_dma_irq);
        }
        disable_events(ls, mask);
        return 0;
    }

    // Enable IRQs.
    mask &= ~ir->inte;
    if (mask == 0) return 0;
    enable_events(ls, mask);
    uint32_t save = save_and_disable_interrupts();
    dma_hw->intr = mask;  // Clear pending IRQs
    dma_state.pending[core] &= ~mask;
    restore_interrupts(save);
    if (ir->inte == 0) {
        mlua_event_set_irq_handler(irq, &handle_dma_irq, priority);
        irq_set_enabled(irq, true);
    }
    hw_set_bits(&ir->inte, mask);
    return 0;
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

MLUA_FUNC_V1(mod_, dma_, channel_claim, check_channel)
MLUA_FUNC_V1(mod_, dma_, claim_mask, luaL_checkinteger)
MLUA_FUNC_V1(mod_, dma_, channel_unclaim, check_channel)
MLUA_FUNC_V1(mod_, dma_, unclaim_mask, luaL_checkinteger)
MLUA_FUNC_R(mod_, dma_, claim_unused_channel, lua_pushinteger,
            mlua_opt_cbool(ls, 1, true))
MLUA_FUNC_R1(mod_, dma_, channel_is_claimed, lua_pushboolean, check_channel)
MLUA_FUNC_V3(mod_, dma_, channel_set_config, check_channel, check_Config,
             mlua_to_cbool)
MLUA_FUNC_V3(mod_, dma_, channel_set_read_addr, check_channel, check_read_addr,
             mlua_to_cbool)
MLUA_FUNC_V3(mod_, dma_, channel_set_write_addr, check_channel,
             check_write_addr, mlua_to_cbool)
MLUA_FUNC_V3(mod_, dma_, channel_set_trans_count, check_channel,
             luaL_checkinteger, mlua_to_cbool)
MLUA_FUNC_V6(mod_, dma_, channel_configure, check_channel, check_Config,
             check_write_addr, check_read_addr, luaL_checkinteger,
             mlua_to_cbool)
MLUA_FUNC_V3(mod_, dma_, channel_transfer_from_buffer_now, check_channel,
             check_read_addr, luaL_checkinteger)
MLUA_FUNC_V3(mod_, dma_, channel_transfer_to_buffer_now, check_channel,
             check_write_addr, luaL_checkinteger)
MLUA_FUNC_V1(mod_, dma_, start_channel_mask, luaL_checkinteger)
MLUA_FUNC_V1(mod_, dma_, channel_start, check_channel)
MLUA_FUNC_V1(mod_, dma_, channel_abort, check_channel)
MLUA_FUNC_R1(mod_, dma_, channel_is_busy, lua_pushboolean, check_channel)
MLUA_FUNC_V1(mod_, dma_, channel_wait_for_finish_blocking, check_channel)
MLUA_FUNC_V3(mod_, dma_, sniffer_enable, check_channel, luaL_checkinteger,
             mlua_to_cbool)
MLUA_FUNC_V1(mod_, dma_, sniffer_set_byte_swap_enabled, mlua_to_cbool)
MLUA_FUNC_V1(mod_, dma_, sniffer_set_output_invert_enabled, mlua_to_cbool)
MLUA_FUNC_V1(mod_, dma_, sniffer_set_output_reverse_enabled, mlua_to_cbool)
MLUA_FUNC_V0(mod_, dma_, sniffer_disable)
MLUA_FUNC_V1(mod_, dma_, sniffer_set_data_accumulator, luaL_checkinteger)
MLUA_FUNC_R0(mod_, dma_, sniffer_get_data_accumulator, lua_pushinteger)
MLUA_FUNC_V1(mod_, dma_, timer_claim, check_timer)
MLUA_FUNC_V1(mod_, dma_, timer_unclaim, check_timer)
MLUA_FUNC_R(mod_, dma_, claim_unused_timer, lua_pushinteger,
            mlua_opt_cbool(ls, 1, true))
MLUA_FUNC_R1(mod_, dma_, timer_is_claimed, lua_pushboolean, check_timer)
MLUA_FUNC_V3(mod_, dma_, timer_set_fraction, check_timer, luaL_checkinteger,
             luaL_checkinteger)
MLUA_FUNC_R1(mod_, dma_, get_timer_dreq, lua_pushinteger, check_timer)
MLUA_FUNC_V1(mod_, dma_, channel_cleanup, check_timer)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(NUM_CHANNELS, integer, NUM_DMA_CHANNELS),
    MLUA_SYM_V(NUM_TIMERS, integer, NUM_DMA_TIMERS),
    MLUA_SYM_V(DREQ_DMA_TIMER0, integer, DREQ_DMA_TIMER0),
    MLUA_SYM_V(DREQ_DMA_TIMER1, integer, DREQ_DMA_TIMER1),
    MLUA_SYM_V(DREQ_DMA_TIMER2, integer, DREQ_DMA_TIMER2),
    MLUA_SYM_V(DREQ_DMA_TIMER3, integer, DREQ_DMA_TIMER3),
    MLUA_SYM_V(DREQ_FORCE, integer, DREQ_FORCE),
    MLUA_SYM_V(SIZE_8, integer, DMA_SIZE_8),
    MLUA_SYM_V(SIZE_16, integer, DMA_SIZE_16),
    MLUA_SYM_V(SIZE_32, integer, DMA_SIZE_32),
    MLUA_SYM_V(SNIFF_CRC32, integer, 0x0),
    MLUA_SYM_V(SNIFF_CRC32_REV, integer, 0x1),
    MLUA_SYM_V(SNIFF_CRC16, integer, 0x2),
    MLUA_SYM_V(SNIFF_CRC16_REV, integer, 0x3),
    MLUA_SYM_V(SNIFF_XOR, integer, 0xe),
    MLUA_SYM_V(SNIFF_ADD, integer, 0xf),

    MLUA_SYM_F(channel_get_default_config, mod_),
    MLUA_SYM_F(get_channel_config, mod_),
    MLUA_SYM_F(regs, mod_),
    MLUA_SYM_F(channel_claim, mod_),
    MLUA_SYM_F(claim_mask, mod_),
    MLUA_SYM_F(channel_unclaim, mod_),
    MLUA_SYM_F(unclaim_mask, mod_),
    MLUA_SYM_F(claim_unused_channel, mod_),
    MLUA_SYM_F(channel_is_claimed, mod_),
    MLUA_SYM_F(channel_set_config, mod_),
    MLUA_SYM_F(channel_set_read_addr, mod_),
    MLUA_SYM_F(channel_set_write_addr, mod_),
    MLUA_SYM_F(channel_set_trans_count, mod_),
    MLUA_SYM_F(channel_configure, mod_),
    MLUA_SYM_F(channel_transfer_from_buffer_now, mod_),
    MLUA_SYM_F(channel_transfer_to_buffer_now, mod_),
    MLUA_SYM_F(start_channel_mask, mod_),
    MLUA_SYM_F(channel_start, mod_),
    MLUA_SYM_F(channel_abort, mod_),
    // channel_set_irq0_enabled: Use enable_irq
    // set_irq0_channel_mask_enabled: Use enable_irq
    // channel_set_irq1_enabled: Use enable_irq
    // set_irq1_channel_mask_enabled: Use enable_irq
    // irqn_set_channel_enabled: Use enable_irq
    // irqn_set_channel_mask_enabled: Use enable_irq
    MLUA_SYM_F(channel_get_irq_status, mod_),
    // channel_get_irq0_status: Use channel_get_irq_status
    // channel_get_irq1_status: Use channel_get_irq_status
    // irqn_get_channel_status: Use channel_get_irq_status
    MLUA_SYM_F(channel_acknowledge_irq, mod_),
    // channel_acknowledge_irq0: Use channel_acknowledge_irq
    // channel_acknowledge_irq1: Use channel_acknowledge_irq
    // irqn_acknowledge_channel: Use channel_acknowledge_irq
    MLUA_SYM_F(channel_is_busy, mod_),
    MLUA_SYM_F(channel_wait_for_finish_blocking, mod_),
    MLUA_SYM_F(sniffer_enable, mod_),
    MLUA_SYM_F(sniffer_set_byte_swap_enabled, mod_),
    MLUA_SYM_F(sniffer_set_output_invert_enabled, mod_),
    MLUA_SYM_F(sniffer_set_output_reverse_enabled, mod_),
    MLUA_SYM_F(sniffer_disable, mod_),
    MLUA_SYM_F(sniffer_set_data_accumulator, mod_),
    MLUA_SYM_F(sniffer_get_data_accumulator, mod_),
    MLUA_SYM_F(timer_claim, mod_),
    MLUA_SYM_F(timer_unclaim, mod_),
    MLUA_SYM_F(claim_unused_timer, mod_),
    MLUA_SYM_F(timer_is_claimed, mod_),
    MLUA_SYM_F(timer_set_fraction, mod_),
    MLUA_SYM_F(get_timer_dreq, mod_),
    MLUA_SYM_F(channel_cleanup, mod_),
    MLUA_SYM_F(wait_irq, mod_),
    MLUA_SYM_F_THREAD(enable_irq, mod_),
};

MLUA_OPEN_MODULE(hardware.dma) {
    mlua_thread_require(ls);

    // Create the module.
    mlua_new_module(ls, NUM_DMA_CHANNELS, module_syms);

    // Create the Config class.
    mlua_new_class(ls, Config_name, Config_syms, mlua_nosyms);
    lua_pop(ls, 1);
    return 1;
}
