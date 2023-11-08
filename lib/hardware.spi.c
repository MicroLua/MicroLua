// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>
#include <stdint.h>

#include "hardware/address_mapped.h"
#include "hardware/irq.h"
#include "hardware/regs/intctrl.h"
#include "hardware/spi.h"
#include "pico/platform.h"

#include "mlua/event.h"
#include "mlua/module.h"
#include "mlua/util.h"

char const mlua_SPI_name[] = "hardware.spi.SPI";

static inline spi_inst_t* get_instance(uint num) {
    return num == 0 ? spi0 : spi1;
}

static spi_inst_t** new_SPI(lua_State* ls) {
    spi_inst_t** v = lua_newuserdatauv(ls, sizeof(spi_inst_t*), 0);
    luaL_getmetatable(ls, mlua_SPI_name);
    lua_setmetatable(ls, -2);
    return v;
}

static inline spi_inst_t* check_SPI(lua_State* ls, int arg) {
    return *((spi_inst_t**)luaL_checkudata(ls, arg, mlua_SPI_name));
}

static inline spi_inst_t* to_SPI(lua_State* ls, int arg) {
    return *((spi_inst_t**)lua_touserdata(ls, arg));
}

static int SPI_regs_base(lua_State* ls) {
    lua_pushinteger(ls, (uintptr_t)spi_get_hw(check_SPI(ls, 1)));
    return 1;
}

#if LIB_MLUA_MOD_MLUA_EVENT

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
    mlua_event_set(state->event);
}

static int SPI_enable_irq(lua_State* ls) {
    spi_inst_t* inst = to_SPI(ls, 1);
    uint num = spi_get_index(inst);
    uint irq = SPI0_IRQ + num;
    SPIState* state = &spi_state[num];
    lua_Integer priority = -1;
    if (!mlua_event_enable_irq_arg(ls, 2, &priority)) {  // Disable IRQ
        irq_set_enabled(irq, false);
        irq_remove_handler(irq, &handle_spi_irq);
        mlua_event_unclaim(ls, &state->event);
        return 0;
    }
    char const* err = mlua_event_claim(&state->event);
    if (err != NULL) return luaL_error(ls, "SPI%d: %s", num, err);
    mlua_event_set_irq_handler(irq, &handle_spi_irq, priority);
    irq_set_enabled(irq, true);
    return 0;
}

#endif  // LIB_MLUA_MOD_MLUA_EVENT

static int SPI_deinit(lua_State* ls) {
    spi_inst_t* inst = check_SPI(ls, 1);
#if LIB_MLUA_MOD_MLUA_EVENT
    lua_pushcfunction(ls, &SPI_enable_irq);
    lua_pushvalue(ls, 1);
    lua_pushboolean(ls, false);
    lua_call(ls, 2, 0);
#endif
    spi_deinit(inst);
    return 0;
}

static int SPI_set_format(lua_State* ls) {
    spi_set_format(check_SPI(ls, 1), luaL_checkinteger(ls, 2),
                   luaL_checkinteger(ls, 3), luaL_checkinteger(ls, 4),
                   luaL_optinteger(ls, 5, SPI_MSB_FIRST));
    return 0;
}

#define WR_FIFO_DEPTH 8

// This function combines all spi_(write(16)?_)(read(16)?_)blocking functions
// into a single implementation. The functions from the SDK are placed in RAM,
// probbaly for performance reasons, but since performance matters less with Lua
// but RAM is scarce, we trade a bit of performance against RAM.
static void write_read_blocking(spi_inst_t* inst, uint8_t const* src,
                                uint8_t* dst, size_t len, bool bits16,
                                uint16_t tx_data) {
    size_t tx = len, rx = len;
    spi_hw_t* hw = spi_get_hw(inst);
    while (tx > 0 || rx > 0) {
        // Feed the TX FIFO, avoiding RX FIFO overflows.
        if (tx > 0 && rx < tx + WR_FIFO_DEPTH && spi_is_writable(inst)) {
            if (src != NULL) {
                tx_data = *src++;
                if (bits16) tx_data |= (*src++) << 8;
            }
            hw->dr = (uint32_t)tx_data;
            --tx;
        }

        // Drain the RX FIFO.
        if (rx > 0 && spi_is_readable(inst)) {
            uint32_t data = hw->dr;
            if (dst != NULL) {
                *dst++ = data;
                if (bits16) *dst++ = data >> 8;
            }
            --rx;
        }
    }
}

enum { wrf_16 = 1, wrf_write = 2, wrf_read = 4 };

static int try_write_read(lua_State* ls, bool timeout) {
    spi_inst_t* inst = to_SPI(ls, 1);
    int flags = lua_tointeger(ls, 3);
    size_t tx = lua_tointeger(ls, 4);
    size_t rx = lua_tointeger(ls, 5);
    bool bits16 = (flags & wrf_16) != 0;
    uint8_t const* src = NULL;
    uint32_t tx_data = 0;
    if ((flags & wrf_write) != 0) {
        size_t len;
        src = (uint8_t const*)lua_tolstring(ls, 2, &len);
        src += len - tx;
        if (bits16) src -= tx;
    } else {
        tx_data = lua_tointeger(ls, 2) & 0xffff;
    }
    luaL_Buffer buf;
    uint8_t* dst = NULL;
    if ((flags & wrf_read) != 0 && rx > 0) {
        dst = (uint8_t*)luaL_buffinitsize(ls, &buf, rx);
    }

    spi_hw_t* hw = spi_get_hw(inst);
    for (;;) {
        bool suspend = true;

        // Feed the TX FIFO, avoiding RX FIFO overflows.
        while (tx > 0 && rx < tx + WR_FIFO_DEPTH && spi_is_writable(inst)) {
            if (src != NULL) {
                tx_data = *src++;
                if (bits16) tx_data |= (*src++) << 8;
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
                if (bits16) {
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
            if (dst != NULL && luaL_bufflen(&buf) > 0) luaL_pushresult(&buf);
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

static int write_read_non_blocking(lua_State* ls, spi_inst_t* inst, int flags,
                                   size_t len) {
    MLuaEvent* event = &spi_state[spi_get_index(inst)].event;
    if (!mlua_event_can_wait(event)) return -1;
    lua_settop(ls, 2);
    lua_pushinteger(ls, flags);  // flags
    lua_pushinteger(ls, len);  // tx
    lua_pushinteger(ls, len);  // rx
    return mlua_event_wait(ls, *event, &try_write_read, 0);
}

static int SPI_write_read_blocking(lua_State* ls) {
    spi_inst_t* inst = to_SPI(ls, 1);
    size_t len;
    uint8_t const* src = (uint8_t const*)luaL_checklstring(ls, 2, &len);

    int res = write_read_non_blocking(ls, inst, wrf_write | wrf_read, len);
    if (res >= 0) return res;

    luaL_Buffer buf;
    uint8_t* dst = (uint8_t*)luaL_buffinitsize(ls, &buf, len);
    write_read_blocking(inst, src, dst, len, false, 0);
    luaL_pushresultsize(&buf, len);
    return 1;
}

static int SPI_write_blocking(lua_State* ls) {
    spi_inst_t* inst = to_SPI(ls, 1);
    size_t len;
    uint8_t const* src = (uint8_t const*)luaL_checklstring(ls, 2, &len);

    int res = write_read_non_blocking(ls, inst, wrf_write, len);
    if (res >= 0) return res;

    write_read_blocking(inst, src, NULL, len, false, 0);
    return 0;
}

static int SPI_read_blocking(lua_State* ls) {
    spi_inst_t* inst = to_SPI(ls, 1);
    uint16_t tx_data = luaL_checkinteger(ls, 2);
    size_t len = luaL_checkinteger(ls, 3);

    int res = write_read_non_blocking(ls, inst, wrf_read, len);
    if (res >= 0) return res;

    luaL_Buffer buf;
    uint8_t* dst = (uint8_t*)luaL_buffinitsize(ls, &buf, len);
    write_read_blocking(inst, NULL, dst, len, false, tx_data);
    luaL_pushresultsize(&buf, len);
    return 1;
}

static int SPI_write16_read16_blocking(lua_State* ls) {
    spi_inst_t* inst = to_SPI(ls, 1);
    size_t len;
    uint8_t const* src = (uint8_t const*)luaL_checklstring(ls, 2, &len);
    luaL_argcheck(ls, len % 2 == 0, 2, "length must be even");

    int res = write_read_non_blocking(ls, inst, wrf_16 | wrf_write | wrf_read,
                                      len / 2);
    if (res >= 0) return res;

    luaL_Buffer buf;
    uint8_t* dst = (uint8_t*)luaL_buffinitsize(ls, &buf, len);
    write_read_blocking(inst, src, dst, len / 2, true, 0);
    luaL_pushresultsize(&buf, len);
    return 1;
}

static int SPI_write16_blocking(lua_State* ls) {
    spi_inst_t* inst = to_SPI(ls, 1);
    size_t len;
    uint8_t const* src = (uint8_t const*)luaL_checklstring(ls, 2, &len);
    luaL_argcheck(ls, len % 2 == 0, 2, "length must be even");
    len /= 2;

    int res = write_read_non_blocking(ls, inst, wrf_16 | wrf_write, len);
    if (res >= 0) return res;

    write_read_blocking(inst, src, NULL, len, true, 0);
    return 0;
}

static int SPI_read16_blocking(lua_State* ls) {
    spi_inst_t* inst = to_SPI(ls, 1);
    uint16_t tx_data = luaL_checkinteger(ls, 2);
    size_t len = luaL_checkinteger(ls, 3);

    int res = write_read_non_blocking(ls, inst, wrf_16 | wrf_read, len);
    if (res >= 0) return res;

    luaL_Buffer buf;
    uint8_t* dst = (uint8_t*)luaL_buffinitsize(ls, &buf, 2 * len);
    write_read_blocking(inst, NULL, dst, len, true, tx_data);
    luaL_pushresultsize(&buf, 2 * len);
    return 1;
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

MLUA_FUNC_1_2(SPI_, spi_, init, lua_pushinteger, check_SPI,
              luaL_checkinteger)
MLUA_FUNC_1_2(SPI_, spi_, set_baudrate, lua_pushinteger, check_SPI,
              luaL_checkinteger)
MLUA_FUNC_1_1(SPI_, spi_, get_baudrate, lua_pushinteger, check_SPI)
MLUA_FUNC_1_1(SPI_, spi_, get_index, lua_pushinteger, check_SPI)
MLUA_FUNC_0_2(SPI_, spi_, set_slave, check_SPI, mlua_to_cbool)
MLUA_FUNC_1_1(SPI_, spi_, is_writable, lua_pushboolean, check_SPI)
MLUA_FUNC_1_1(SPI_, spi_, is_readable, lua_pushboolean, check_SPI)
MLUA_FUNC_1_1(SPI_, spi_, is_busy, lua_pushboolean, check_SPI)
MLUA_FUNC_1_2(SPI_, spi_, get_dreq, lua_pushinteger, check_SPI, mlua_to_cbool)

MLUA_SYMBOLS(SPI_syms) = {
    MLUA_SYM_F(init, SPI_),
    MLUA_SYM_F(deinit, SPI_),
    MLUA_SYM_F(set_baudrate, SPI_),
    MLUA_SYM_F(get_baudrate, SPI_),
    MLUA_SYM_F(get_index, SPI_),
    MLUA_SYM_F(regs_base, SPI_),
    MLUA_SYM_F(set_format, SPI_),
    MLUA_SYM_F(set_slave, SPI_),
    MLUA_SYM_F(is_writable, SPI_),
    MLUA_SYM_F(is_readable, SPI_),
    MLUA_SYM_F(is_busy, SPI_),
    MLUA_SYM_F(write_read_blocking, SPI_),
    MLUA_SYM_F(write_blocking, SPI_),
    MLUA_SYM_F(read_blocking, SPI_),
    MLUA_SYM_F(write16_read16_blocking, SPI_),
    MLUA_SYM_F(write16_blocking, SPI_),
    MLUA_SYM_F(read16_blocking, SPI_),
    MLUA_SYM_F(get_dreq, SPI_),
    MLUA_SYM_F(enable_loopback, SPI_),
#if LIB_MLUA_MOD_MLUA_EVENT
    MLUA_SYM_F(enable_irq, SPI_),
#else
    MLUA_SYM_V(enable_irq, boolean, false),
#endif
};

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(CPHA_0, integer, SPI_CPHA_0),
    MLUA_SYM_V(CPHA_1, integer, SPI_CPHA_1),
    MLUA_SYM_V(CPOL_0, integer, SPI_CPOL_0),
    MLUA_SYM_V(CPOL_1, integer, SPI_CPOL_1),
    MLUA_SYM_V(LSB_FIRST, integer, SPI_LSB_FIRST),
    MLUA_SYM_V(MSB_FIRST, integer, SPI_MSB_FIRST),
    MLUA_SYM_V(NUM, integer, NUM_SPIS),
    MLUA_SYM_V(_default, boolean, false),
};

#if LIB_MLUA_MOD_MLUA_EVENT

static __attribute__((constructor)) void init(void) {
    for (uint i = 0; i < NUM_SPIS; ++i) {
        SPIState* state = &spi_state[i];
        state->event = MLUA_EVENT_UNSET;
    }
}

#endif  // LIB_MLUA_MOD_MLUA_EVENT

MLUA_OPEN_MODULE(hardware.spi) {
    mlua_event_require(ls);

    // Create the module.
    mlua_new_module(ls, NUM_SPIS, module_syms);
    int mod_index = lua_gettop(ls);

    // Create the SPI class.
    mlua_new_class(ls, mlua_SPI_name, SPI_syms, true);
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
