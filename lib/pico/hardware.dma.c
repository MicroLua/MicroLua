// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/address_mapped.h"
#include "hardware/dma.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"
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

    MLUA_SYM_F(channel_get_default_config, mod_),
    MLUA_SYM_F(get_channel_config, mod_),
    MLUA_SYM_F(regs, mod_),
};

MLUA_OPEN_MODULE(hardware.dma) {
    // Create the module.
    mlua_new_module(ls, NUM_DMA_CHANNELS, module_syms);

    // Create the Config class.
    mlua_new_class(ls, Config_name, Config_syms, true);
    lua_pop(ls, 1);
    return 1;
}
