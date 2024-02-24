// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/pio.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"
#include "mlua/util.h"

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

// TODO: Make count argument optional for set_(out|set)_pins

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
SM_FUNC_V1(put_blocking, luaL_checkinteger)
SM_FUNC_R0(get_blocking, lua_pushinteger)
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

static inline PIO get_instance(uint num) {
    return num == 0 ? pio0 : pio1;
}

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
    PIO pio = check_PIO(ls, 1);
    uint num = luaL_checkinteger(ls, 2);
    luaL_argcheck(ls, num < NUM_PIO_STATE_MACHINES, 2,
                  "invalid state machine number");
    if (lua_getiuservalue(ls, 1, num + 1) == LUA_TNIL) {
        lua_pop(ls, 1);
        SM* sm = new_SM(ls);
        sm->pio = pio;
        sm->sm = num;
        lua_pushvalue(ls, -1);
        lua_setiuservalue(ls, 1, num + 1);
    }
    return 1;
}

static int PIO_regs_base(lua_State* ls) {
    lua_pushinteger(ls, (uintptr_t)check_PIO(ls, 1));
    return 1;
}

static void to_program(lua_State* ls, pio_program_t* prog) {
    prog->length = lua_rawlen(ls, 2);
    if (prog->length > 32) luaL_error(ls, "program too large");
    if (lua_getfield(ls, 2, "origin") != LUA_TNIL) {
        prog->origin = luaL_checkinteger(ls, -1);
    }
    lua_pop(ls, 1);
}

static int PIO_can_add_program(lua_State* ls) {
    PIO pio = check_PIO(ls, 1);
    luaL_checktype(ls, 2, LUA_TTABLE);
    pio_program_t prog;
    to_program(ls, &prog);
    return lua_pushboolean(ls, pio_can_add_program(pio, &prog)), 1;
}

static int PIO_can_add_program_at_offset(lua_State* ls) {
    PIO pio = check_PIO(ls, 1);
    luaL_checktype(ls, 2, LUA_TTABLE);
    uint off = luaL_checkinteger(ls, 3);
    pio_program_t prog;
    to_program(ls, &prog);
    lua_pushboolean(ls, pio_can_add_program_at_offset(pio, &prog, off));
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
    PIO pio = check_PIO(ls, 1);
    luaL_checktype(ls, 2, LUA_TTABLE);
    pio_program_t prog;
    to_program(ls, &prog);
    uint16_t instr[prog.length];
    prog.instructions = instr;
    copy_instructions(ls, prog.length, instr);
    return lua_pushinteger(ls, pio_add_program(pio, &prog)), 1;
}

static int PIO_add_program_at_offset(lua_State* ls) {
    PIO pio = check_PIO(ls, 1);
    luaL_checktype(ls, 2, LUA_TTABLE);
    uint off = luaL_checkinteger(ls, 3);
    pio_program_t prog;
    to_program(ls, &prog);
    uint16_t instr[prog.length];
    prog.instructions = instr;
    copy_instructions(ls, prog.length, instr);
    pio_add_program_at_offset(pio, &prog, off);
    return 0;
}

static int PIO_remove_program(lua_State* ls) {
    PIO pio = check_PIO(ls, 1);
    pio_program_t prog;
    if (lua_type(ls, 2) == LUA_TTABLE) {
        prog.length = luaL_len(ls, 2);
    } else if (lua_isinteger(ls, 2)) {
        prog.length = lua_tointeger(ls, 2);
    } else {
        return luaL_typeerror(ls, 2, "table or integer");
    }
    uint off = luaL_checkinteger(ls, 3);
    pio_remove_program(pio, &prog, off);
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
MLUA_FUNC_V3(PIO_, pio_, set_irq0_source_enabled, check_PIO, luaL_checkinteger,
             mlua_to_cbool)
MLUA_FUNC_V3(PIO_, pio_, set_irq1_source_enabled, check_PIO, luaL_checkinteger,
             mlua_to_cbool)
MLUA_FUNC_V3(PIO_, pio_, set_irq0_source_mask_enabled, check_PIO,
             luaL_checkinteger, mlua_to_cbool)
MLUA_FUNC_V3(PIO_, pio_, set_irq1_source_mask_enabled, check_PIO,
             luaL_checkinteger, mlua_to_cbool)
MLUA_FUNC_V4(PIO_, pio_, set_irqn_source_enabled, check_PIO, luaL_checkinteger,
             luaL_checkinteger, mlua_to_cbool)
MLUA_FUNC_V4(PIO_, pio_, set_irqn_source_mask_enabled, check_PIO,
             luaL_checkinteger, luaL_checkinteger, mlua_to_cbool)
MLUA_FUNC_R2(PIO_, pio_, interrupt_get, lua_pushboolean, check_PIO,
             luaL_checkinteger)
MLUA_FUNC_V2(PIO_, pio_, interrupt_clear, check_PIO, luaL_checkinteger)
MLUA_FUNC_V2(PIO_, pio_, claim_sm_mask, check_PIO, luaL_checkinteger)
MLUA_FUNC_R2(PIO_, pio_, claim_unused_sm, lua_pushinteger, check_PIO,
             mlua_to_cbool)

MLUA_SYMBOLS(PIO_syms) = {
    MLUA_SYM_F(sm, PIO_),
    MLUA_SYM_F(get_index, PIO_),
    MLUA_SYM_F(regs_base, PIO_),
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
    MLUA_SYM_F(set_irq0_source_enabled, PIO_),
    MLUA_SYM_F(set_irq1_source_enabled, PIO_),
    MLUA_SYM_F(set_irq0_source_mask_enabled, PIO_),
    MLUA_SYM_F(set_irq1_source_mask_enabled, PIO_),
    MLUA_SYM_F(set_irqn_source_enabled, PIO_),
    MLUA_SYM_F(set_irqn_source_mask_enabled, PIO_),
    MLUA_SYM_F(interrupt_get, PIO_),
    MLUA_SYM_F(interrupt_clear, PIO_),
    MLUA_SYM_F(claim_sm_mask, PIO_),
    MLUA_SYM_F(claim_unused_sm, PIO_),
};

static int mod_get_default_sm_config(lua_State* ls) {
    pio_sm_config* cfg = lua_newuserdatauv(ls, sizeof(pio_sm_config), 0);
    luaL_getmetatable(ls, Config_name);
    lua_setmetatable(ls, -2);
    *cfg = pio_get_default_sm_config();
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(NUM, integer, NUM_SPIS),
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
        *new_PIO(ls) = get_instance(i);
        lua_seti(ls, mod_index, i);
    }
    return 1;
}
