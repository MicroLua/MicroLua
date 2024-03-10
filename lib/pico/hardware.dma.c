// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/address_mapped.h"
#include "hardware/dma.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/mem.h"
#include "mlua/module.h"
#include "mlua/util.h"

// TODO: Add support for IRQs

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
    // TODO: Add support for array.int(8|16|32)
    // TODO: Call addr() if it is present, or maybe an __addr metamethod, or
    //       even __raddr for read and __waddr for write
    if (lua_isnil(ls, arg)) return NULL;
    if (lua_isinteger(ls, arg)) {
        return (void const*)(uintptr_t)luaL_checkinteger(ls, arg);
    }
    size_t len;
    void const* ptr = lua_tolstring(ls, arg, &len);
    if (ptr != NULL) return ptr;
    ptr = mlua_mem_test_Buffer(ls, arg);
    if (ptr != NULL) return ptr;
    luaL_typeerror(ls, arg, "nil, integer, string or Buffer");
    return NULL;
}

static void* check_write_addr(lua_State* ls, int arg) {
    // TODO: Add support for array.int(8|16|32)
    if (lua_isnil(ls, arg)) return NULL;
    if (lua_isinteger(ls, arg)) {
        return (void*)(uintptr_t)luaL_checkinteger(ls, arg);
    }
    void* ptr = mlua_mem_test_Buffer(ls, arg);
    if (ptr != NULL) return ptr;
    luaL_typeerror(ls, arg, "nil, integer or Buffer");
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
    return (dma_channel_config*)luaL_checkudata(ls, arg, Config_name);
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

static int mod_channel_get_default_config(lua_State* ls) {
    *new_Config(ls) = dma_channel_get_default_config(check_channel(ls, 1));
    return 1;
}

static int mod_get_channel_config(lua_State* ls) {
    *new_Config(ls) = dma_channel_get_default_config(check_channel(ls, 1));
    return 1;
}

static int mod_regs(lua_State* ls) {
    lua_pushinteger(ls, lua_isnoneornil(ls, 1) ? (uintptr_t)dma_hw
                        : (uintptr_t)&dma_hw->ch[check_channel(ls, 1)]);
    return 1;
}

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
    // channel_set_irq0_enabled: Not useful in Lua
    // set_irq0_channel_mask_enabled: Not useful in Lua
    // channel_set_irq1_enabled: Not useful in Lua
    // set_irq1_channel_mask_enabled: Not useful in Lua
    // irqn_set_channel_enabled: Not useful in Lua
    // irqn_set_channel_mask_enabled: Not useful in Lua
    // TODO: MLUA_SYM_F(get_irq0_status, mod_),
    // TODO: MLUA_SYM_F(get_irq1_status, mod_),
    // TODO: MLUA_SYM_F(irqn_get_channel_status, mod_),
    // TODO: MLUA_SYM_F(channel_acknowledge_irq0, mod_),
    // TODO: MLUA_SYM_F(channel_acknowledge_irq1, mod_),
    // TODO: MLUA_SYM_F(irqn_acknowledge_channel, mod_),
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
};

MLUA_OPEN_MODULE(hardware.dma) {
    // Create the module.
    mlua_new_module(ls, NUM_DMA_CHANNELS, module_syms);

    // Create the Config class.
    mlua_new_class(ls, Config_name, Config_syms, true);
    lua_pop(ls, 1);
    return 1;
}
