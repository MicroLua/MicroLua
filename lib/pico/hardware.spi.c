// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>
#include <stdint.h>

#include "hardware/address_mapped.h"
#include "hardware/irq.h"
#include "hardware/regs/intctrl.h"
#include "hardware/spi.h"
#include "pico.h"

#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

static char const SPI_name[] = "hardware.spi.SPI";

static inline spi_inst_t* get_instance(uint num) {
    return num == 0 ? spi0 : spi1;
}

static inline bool use_16_bit_values(spi_hw_t* hw) {
    return (hw->cr0 & SPI_SSPCR0_DSS_BITS) >> SPI_SSPCR0_DSS_LSB > 7;
}

static spi_inst_t** new_SPI(lua_State* ls) {
    spi_inst_t** v = lua_newuserdatauv(ls, sizeof(spi_inst_t*), 0);
    luaL_getmetatable(ls, SPI_name);
    lua_setmetatable(ls, -2);
    return v;
}

static inline spi_inst_t* check_SPI(lua_State* ls, int arg) {
    return *((spi_inst_t**)luaL_checkudata(ls, arg, SPI_name));
}

static inline spi_inst_t* to_SPI(lua_State* ls, int arg) {
    return *((spi_inst_t**)lua_touserdata(ls, arg));
}

static int SPI_regs(lua_State* ls) {
    return lua_pushlightuserdata(ls, spi_get_hw(check_SPI(ls, 1))), 1;
}

#if LIB_MLUA_MOD_MLUA_THREAD

typedef struct SPIState {
    MLuaEvent event;
} SPIState;

static SPIState spi_state[NUM_SPIS];

static void __time_critical_func(handle_spi_irq)(void) {
    uint num = __get_current_exception() - VTABLE_FIRST_IRQ - SPI0_IRQ;
    spi_hw_t* hw = spi_get_hw(get_instance(num));
    hw_clear_bits(&hw->imsc,
        SPI_SSPIMSC_TXIM_BITS | SPI_SSPIMSC_RXIM_BITS | SPI_SSPIMSC_RTIM_BITS);
    SPIState* state = &spi_state[num];
    mlua_event_set(&state->event);
}

static int SPI_enable_irq(lua_State* ls) {
    spi_inst_t* inst = check_SPI(ls, 1);
    uint num = spi_get_index(inst);
    uint irq = SPI0_IRQ + num;
    SPIState* state = &spi_state[num];
    lua_Integer priority = -1;
    if (!mlua_event_enable_irq_arg(ls, 2, &priority)) {  // Disable IRQ
        irq_set_enabled(irq, false);
        irq_remove_handler(irq, &handle_spi_irq);
        mlua_event_disable(ls, &state->event);
        return 0;
    }
    if (!mlua_event_enable(ls, &state->event)) {
        return luaL_error(ls, "SPI%d: IRQ already enabled", num);
    }
    mlua_event_set_irq_handler(irq, &handle_spi_irq, priority);
    irq_set_enabled(irq, true);
    return 0;
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

static int SPI_deinit(lua_State* ls) {
    spi_inst_t* inst = check_SPI(ls, 1);
#if LIB_MLUA_MOD_MLUA_THREAD
    lua_pushcfunction(ls, &SPI_enable_irq);
    lua_pushvalue(ls, 1);
    lua_pushboolean(ls, false);
    lua_call(ls, 2, 0);
#endif
    spi_deinit(inst);
    return 0;
}

#define WR_FIFO_DEPTH 8

// This function combines all spi_(write(16)?_)(read(16)?_)blocking functions
// into a single implementation. The functions from the SDK are placed in RAM,
// probbaly for performance reasons, but since performance matters less with Lua
// but RAM is scarce, we trade a bit of performance against RAM.
static int write_read_blocking(lua_State* ls, spi_inst_t* inst, bool read,
                               uint8_t const* src, size_t len) {
    size_t tx = len, rx = len;
    spi_hw_t* hw = spi_get_hw(inst);
    bool b16 = use_16_bit_values(hw);
    uint16_t tx_data = src == NULL ? lua_tointeger(ls, 2) : 0;
    luaL_Buffer buf;
    uint8_t* dst = NULL;
    if (read) {
        if (b16) len *= 2;
        dst = (uint8_t*)luaL_buffinitsize(ls, &buf, len);
    }
    while (tx > 0 || rx > 0) {
        // Feed the TX FIFO, avoiding RX FIFO overflows.
        if (tx > 0 && rx < tx + WR_FIFO_DEPTH && spi_is_writable(inst)) {
            if (src != NULL) {
                tx_data = *src++;
                if (b16) tx_data |= (*src++) << 8;
            }
            hw->dr = (uint32_t)tx_data;
            --tx;
        }

        // Drain the RX FIFO.
        if (rx > 0 && spi_is_readable(inst)) {
            uint32_t data = hw->dr;
            if (dst != NULL) {
                *dst++ = data;
                if (b16) *dst++ = data >> 8;
            }
            --rx;
        }
    }
    if (dst == NULL) return 0;
    luaL_pushresultsize(&buf, len);
    return 1;
}

static int write_read_loop(lua_State* ls, bool timeout) {
    spi_inst_t* inst = to_SPI(ls, 1);
    bool read = lua_toboolean(ls, 3);
    size_t tx = lua_tointeger(ls, 4);
    size_t rx = lua_tointeger(ls, 5);
    spi_hw_t* hw = spi_get_hw(inst);
    bool b16 = use_16_bit_values(hw);
    uint8_t const* src = NULL;
    uint32_t tx_data = 0;
    if (lua_isstring(ls, 2)) {
        size_t len;
        src = (uint8_t const*)lua_tolstring(ls, 2, &len);
        src += len - tx;
        if (b16) src -= tx;
    } else {
        tx_data = lua_tointeger(ls, 2) & 0xffff;
    }
    luaL_Buffer buf;
    uint8_t* dst = NULL;
    if (read && rx > 0) {
        dst = (uint8_t*)luaL_buffinitsize(ls, &buf, rx);
    }

    for (;;) {
        bool suspend = true;

        // Feed the TX FIFO, avoiding RX FIFO overflows.
        while (tx > 0 && rx < tx + WR_FIFO_DEPTH && spi_is_writable(inst)) {
            if (src != NULL) {
                tx_data = *src++;
                if (b16) tx_data |= (*src++) << 8;
            }
            hw->dr = tx_data;
            --tx;
            suspend = false;
        }

        // Drain the RX FIFO.
        while (rx > 0 && spi_is_readable(inst)) {
            uint32_t data = hw->dr;
            if (dst != NULL) {
                *dst++ = data;
                luaL_addsize(&buf, 1);
                if (b16) {
                    *dst++ = data >> 8;
                    luaL_addsize(&buf, 1);
                }
            }
            --rx;
            suspend = false;
        }
        if (tx == 0 && rx == 0) break;

        // If nothing was done during this round, suspend and wait for an IRQ.
        if (suspend) {
            hw_set_bits(&hw->imsc,
                (tx > 0 && rx < tx + WR_FIFO_DEPTH ? SPI_SSPIMSC_TXIM_BITS : 0)
                | (rx > 0 ? SPI_SSPIMSC_RXIM_BITS | SPI_SSPIMSC_RTIM_BITS : 0));
            if (dst != NULL && luaL_bufflen(&buf) > 0) {
                luaL_pushresult(&buf);
                if (lua_gettop(ls) >= LUA_MINSTACK - 1) {
                    lua_concat(ls, lua_gettop(ls) - 5);
                }
            }
            lua_pushinteger(ls, tx);
            lua_replace(ls, 4);
            lua_pushinteger(ls, rx);
            lua_replace(ls, 5);
            return -1;
        }
    }
    if (dst == NULL) return 0;
    if (luaL_bufflen(&buf) > 0) luaL_pushresult(&buf);
    lua_concat(ls, lua_gettop(ls) - 5);
    return 1;
}

static int write_read_non_blocking(lua_State* ls, spi_inst_t* inst, bool read,
                                   size_t len) {
    MLuaEvent* event = &spi_state[spi_get_index(inst)].event;
    if (!mlua_event_can_wait(ls, event, 0)) return -1;
    lua_settop(ls, 2);
    lua_pushboolean(ls, read);  // read
    lua_pushinteger(ls, len);  // tx
    lua_pushinteger(ls, len);  // rx
    return mlua_event_wait(ls, event, 0, &write_read_loop, 0);
}

static int write_read(lua_State* ls, bool read, uint8_t const* src,
                      size_t len) {
    spi_inst_t* inst = to_SPI(ls, 1);
    if (src != NULL && use_16_bit_values(spi_get_hw(inst))) {
        luaL_argcheck(ls, len % 2 == 0, 2, "length must be even");
        len /= 2;
    }
    int res = write_read_non_blocking(ls, inst, read, len);
    if (res >= 0) return res;
    return write_read_blocking(ls, inst, read, src, len);
}

static int SPI_write_read_blocking(lua_State* ls) {
    size_t len;
    uint8_t const* src = (uint8_t const*)luaL_checklstring(ls, 2, &len);
    return write_read(ls, true, src, len);
}

static int SPI_write_blocking(lua_State* ls) {
    size_t len;
    uint8_t const* src = (uint8_t const*)luaL_checklstring(ls, 2, &len);
    return write_read(ls, false, src, len);
}

static int SPI_read_blocking(lua_State* ls) {
    luaL_argexpected(ls, lua_isinteger(ls, 2), 2, "integer");
    size_t len = luaL_checkinteger(ls, 3);
    return write_read(ls, true, NULL, len);
}

static int SPI_enable_loopback(lua_State* ls) {
    spi_inst_t* inst = check_SPI(ls, 1);
    if (mlua_to_cbool(ls, 2)) {
        hw_set_bits(&spi_get_hw(inst)->cr1, SPI_SSPCR1_LBM_BITS);
    } else {
        hw_clear_bits(&spi_get_hw(inst)->cr1, SPI_SSPCR1_LBM_BITS);
    }
    return 0;
}

MLUA_FUNC_R2(SPI_, spi_, init, lua_pushinteger, check_SPI, luaL_checkinteger)
MLUA_FUNC_R2(SPI_, spi_, set_baudrate, lua_pushinteger, check_SPI,
             luaL_checkinteger)
MLUA_FUNC_R1(SPI_, spi_, get_baudrate, lua_pushinteger, check_SPI)
MLUA_FUNC_R1(SPI_, spi_, get_index, lua_pushinteger, check_SPI)
MLUA_FUNC_V(SPI_, spi_, set_format, check_SPI(ls, 1), luaL_checkinteger(ls, 2),
            luaL_checkinteger(ls, 3), luaL_checkinteger(ls, 4),
            luaL_optinteger(ls, 5, SPI_MSB_FIRST))
MLUA_FUNC_V2(SPI_, spi_, set_slave, check_SPI, mlua_to_cbool)
MLUA_FUNC_R1(SPI_, spi_, is_writable, lua_pushboolean, check_SPI)
MLUA_FUNC_R1(SPI_, spi_, is_readable, lua_pushboolean, check_SPI)
MLUA_FUNC_R1(SPI_, spi_, is_busy, lua_pushboolean, check_SPI)
MLUA_FUNC_R2(SPI_, spi_, get_dreq, lua_pushinteger, check_SPI, mlua_to_cbool)

MLUA_SYMBOLS(SPI_syms) = {
    MLUA_SYM_F(init, SPI_),
    MLUA_SYM_F(deinit, SPI_),
    MLUA_SYM_F(set_baudrate, SPI_),
    MLUA_SYM_F(get_baudrate, SPI_),
    MLUA_SYM_F(get_index, SPI_),
    MLUA_SYM_F(regs, SPI_),
    MLUA_SYM_F(set_format, SPI_),
    MLUA_SYM_F(set_slave, SPI_),
    MLUA_SYM_F(is_writable, SPI_),
    MLUA_SYM_F(is_readable, SPI_),
    MLUA_SYM_F(is_busy, SPI_),
    MLUA_SYM_F(write_read_blocking, SPI_),
    MLUA_SYM_F(write_blocking, SPI_),
    MLUA_SYM_F(read_blocking, SPI_),
    MLUA_SYM_F(get_dreq, SPI_),
    MLUA_SYM_F(enable_loopback, SPI_),
    MLUA_SYM_F_THREAD(enable_irq, SPI_),
};

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(NUM, integer, NUM_SPIS),
    MLUA_SYM_V(_default, boolean, false),
    MLUA_SYM_V(CPHA_0, integer, SPI_CPHA_0),
    MLUA_SYM_V(CPHA_1, integer, SPI_CPHA_1),
    MLUA_SYM_V(CPOL_0, integer, SPI_CPOL_0),
    MLUA_SYM_V(CPOL_1, integer, SPI_CPOL_1),
    MLUA_SYM_V(LSB_FIRST, integer, SPI_LSB_FIRST),
    MLUA_SYM_V(MSB_FIRST, integer, SPI_MSB_FIRST),
};

MLUA_OPEN_MODULE(hardware.spi) {
    mlua_thread_require(ls);

    // Create the module.
    mlua_new_module(ls, NUM_SPIS, module_syms);
    int mod_index = lua_gettop(ls);

    // Create the SPI class.
    mlua_new_class(ls, SPI_name, SPI_syms, mlua_nosyms);
    lua_pop(ls, 1);

    // Create SPI instances.
    for (uint i = 0; i < NUM_SPIS; ++i) {
        spi_inst_t* inst = get_instance(i);
        *new_SPI(ls) = inst;
#ifdef spi_default
        if (inst == spi_default) {
            lua_pushvalue(ls, -1);
            lua_setfield(ls, mod_index, "default");
        }
#endif
        lua_seti(ls, mod_index, i);
    }
    return 1;
}
