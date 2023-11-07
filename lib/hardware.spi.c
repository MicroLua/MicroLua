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

static spi_inst_t* get_instance(uint num) {
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
    spi_hw_t* hs = spi_get_hw(get_instance(num));
    (void)hs;
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

// This function combines all spi_(write(16)?_)(read(16)?_)blocking functions
// into a single implementation. The functions from the SDK are placed in RAM,
// probbaly for performance reasons, but since performance matters less with Lua
// but RAM is scarce, we trade a bit of performance against RAM.
static void write_read_blocking(spi_inst_t* spi, uint8_t const* src,
                                uint8_t* dst, size_t len, bool bits16,
                                uint16_t tx_data) {
    size_t const fifo_depth = 8;
    size_t rx = len, tx = len;
    spi_hw_t* hw = spi_get_hw(spi);
    while (rx > 0 || tx > 0) {
        if (tx > 0 && spi_is_writable(spi) && rx < tx + fifo_depth) {
            if (src != NULL) {
                tx_data = *src++;
                if (bits16) tx_data |= (*src++) << 8;
            }
            hw->dr = (uint32_t)tx_data;
            --tx;
        }
        if (rx > 0 && spi_is_readable(spi)) {
            uint32_t data = hw->dr;
            if (dst != NULL) {
                *dst++ = data;
                if (bits16) *dst++ = data >> 8;
            }
            --rx;
        }
    }
}

static int SPI_write_read_blocking(lua_State* ls) {
    spi_inst_t* inst = to_SPI(ls, 1);
    size_t len;
    uint8_t const* src = (uint8_t const*)luaL_checklstring(ls, 2, &len);
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
    write_read_blocking(inst, src, NULL, len, false, 0);
    return 0;
}

static int SPI_read_blocking(lua_State* ls) {
    spi_inst_t* inst = to_SPI(ls, 1);
    uint16_t tx_data = luaL_checkinteger(ls, 2);
    size_t len = luaL_checkinteger(ls, 3);
    if (len <= 0) {
        lua_pushliteral(ls, "");
        return 1;
    }
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
    write_read_blocking(inst, src, NULL, len / 2, true, 0);
    return 0;
}

static int SPI_read16_blocking(lua_State* ls) {
    spi_inst_t* inst = to_SPI(ls, 1);
    uint16_t tx_data = luaL_checkinteger(ls, 2);
    size_t len = luaL_checkinteger(ls, 3);
    if (len <= 0) {
        lua_pushliteral(ls, "");
        return 1;
    }
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
