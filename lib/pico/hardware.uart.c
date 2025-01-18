// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/hardware.uart.h"

#include <stdbool.h>
#include <stdint.h>

#include "hardware/address_mapped.h"
#include "hardware/irq.h"
#include "hardware/regs/intctrl.h"
#include "pico.h"
#include "pico/time.h"

#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

// TODO: Accept and return mutable buffers when writing and reading

char const mlua_UART_name[] = "hardware.uart.UART";

static uart_inst_t** new_UART(lua_State* ls) {
    uart_inst_t** v = lua_newuserdatauv(ls, sizeof(uart_inst_t*), 0);
    luaL_getmetatable(ls, mlua_UART_name);
    lua_setmetatable(ls, -2);
    return v;
}

static inline uart_inst_t* to_UART(lua_State* ls, int arg) {
    return *((uart_inst_t**)lua_touserdata(ls, arg));
}

static int UART_regs(lua_State* ls) {
    return lua_pushlightuserdata(ls, uart_get_hw(mlua_check_UART(ls, 1))), 1;
}

static int UART_is_tx_busy(lua_State* ls) {
    uart_inst_t* inst = mlua_check_UART(ls, 1);
    lua_pushboolean(ls, (uart_get_hw(inst)->fr & UART_UARTFR_BUSY_BITS) != 0);
    return 1;
}

static int UART_tx_wait_blocking_1(lua_State* ls, int status, lua_KContext ctx);

static int UART_tx_wait_blocking(lua_State* ls) {
    uart_inst_t* inst = mlua_check_UART(ls, 1);
    if (!mlua_thread_blocking(ls)) {
        return UART_tx_wait_blocking_1(ls, LUA_OK, (lua_KContext)inst);
    }
    uart_tx_wait_blocking(inst);
    return 0;
}

#if LIB_MLUA_MOD_MLUA_THREAD

static int UART_tx_wait_blocking_1(lua_State* ls, int status,
                                   lua_KContext ctx) {
    uart_inst_t* inst = (uart_inst_t*)ctx;
    // Busy loop, same as uart_tx_wait_blocking.
    if ((uart_get_hw(inst)->fr & UART_UARTFR_BUSY_BITS) == 0) return 0;
    return mlua_thread_yield(ls, 0, &UART_tx_wait_blocking_1, ctx);
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

#if LIB_MLUA_MOD_MLUA_THREAD

typedef struct UARTState {
    MLuaEvent rx_event;
    MLuaEvent tx_event;
} UARTState;

static UARTState uart_state[NUM_UARTS];

static void __time_critical_func(handle_uart_irq)(void) {
    uint num = __get_current_exception() - VTABLE_FIRST_IRQ - UART0_IRQ;
    uart_hw_t* hu = uart_get_hw(uart_get_instance(num));
    uint32_t mis = hu->mis;
    hw_clear_bits(&hu->imsc, mis & (
        UART_UARTMIS_RXMIS_BITS | UART_UARTMIS_RTMIS_BITS
        | UART_UARTMIS_TXMIS_BITS));
    UARTState* state = &uart_state[num];
    if (mis & (UART_UARTMIS_RXMIS_BITS | UART_UARTMIS_RTMIS_BITS)) {
        mlua_event_set(&state->rx_event);
    }
    if (mis & UART_UARTMIS_TXMIS_BITS) {
        mlua_event_set(&state->tx_event);
    }
}

static int UART_enable_irq(lua_State* ls) {
    uart_inst_t* inst = mlua_check_UART(ls, 1);
    uint num = uart_get_index(inst);
    uint irq = UART0_IRQ + num;
    UARTState* state = &uart_state[num];
    lua_Integer priority = -1;
    if (!mlua_event_enable_irq_arg(ls, 2, &priority)) {  // Disable IRQ
        irq_set_enabled(irq, false);
        irq_remove_handler(irq, &handle_uart_irq);
        mlua_event_disable(ls, &state->rx_event);
        mlua_event_disable(ls, &state->tx_event);
        return 0;
    }
    if (!mlua_event_enable(ls, &state->rx_event)) {
        return luaL_error(ls, "UART%d: Rx IRQ already enabled", num);
    }
    if (!mlua_event_enable(ls, &state->tx_event)) {
        mlua_event_disable(ls, &state->rx_event);
        return luaL_error(ls, "UART%d: Tx IRQ already enabled", num);
    }
    hw_write_masked(&uart_get_hw(inst)->ifls,  // Thresholds: 1/2 FIFO capacity
                    (2 << UART_UARTIFLS_RXIFLSEL_LSB)
                    | (2 << UART_UARTIFLS_TXIFLSEL_LSB),
                    UART_UARTIFLS_RXIFLSEL_BITS | UART_UARTMIS_TXMIS_BITS);
    mlua_event_set_irq_handler(irq, &handle_uart_irq, priority);
    irq_set_enabled(irq, true);
    return 0;
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

static int UART_deinit(lua_State* ls) {
    uart_inst_t* inst = mlua_check_UART(ls, 1);
#if LIB_MLUA_MOD_MLUA_THREAD
    lua_pushcfunction(ls, &UART_enable_irq);
    lua_pushvalue(ls, 1);
    lua_pushboolean(ls, false);
    lua_call(ls, 2, 0);
#endif
    uart_deinit(inst);
    return 0;
}

static inline void enable_tx_irq(uart_inst_t* inst) {
    hw_set_bits(&uart_get_hw(inst)->imsc, UART_UARTIMSC_TXIM_BITS);
}

static int write_loop(lua_State* ls, bool timeout) {
    uart_inst_t* inst = to_UART(ls, 1);
    size_t len;
    uint8_t const* src = (uint8_t const*)lua_tolstring(ls, 2, &len);
    size_t offset = lua_tointeger(ls, 3);
    lua_pop(ls, 1);
    while (offset < len) {
        if (!uart_is_writable(inst)) {
            lua_pushinteger(ls, offset);
            enable_tx_irq(inst);
            return -1;
        }
        uart_get_hw(inst)->dr = src[offset];
        ++offset;
    }
    return 0;
}

static int UART_write_blocking(lua_State* ls) {
    uart_inst_t* inst = mlua_check_UART(ls, 1);
    size_t len;
    uint8_t const* src = (uint8_t const*)luaL_checklstring(ls, 2, &len);
    if (len == 0) return 0;
    MLuaEvent* event = &uart_state[uart_get_index(inst)].tx_event;
    if (mlua_event_can_wait(ls, event, 0)) {
        lua_settop(ls, 2);
        lua_pushinteger(ls, 0);  // offset
        return mlua_event_wait(ls, event, 0, &write_loop, 0);
    }
    uart_write_blocking(inst, src, len);
    return 0;
}

static inline void enable_rx_irq(uart_inst_t* inst) {
    hw_set_bits(&uart_get_hw(inst)->imsc,
                UART_UARTIMSC_RXIM_BITS | UART_UARTIMSC_RTIM_BITS);
}

static int read_loop(lua_State* ls, bool timeout) {
    uart_inst_t* inst = to_UART(ls, 1);
    size_t len = lua_tointeger(ls, 2);
    size_t offset = lua_tointeger(ls, 3);
    luaL_Buffer buf;
    luaL_buffinit(ls, &buf);
    while (offset < len) {
        if (!uart_is_readable(inst)) {
            luaL_pushresult(&buf);
            if (lua_gettop(ls) >= LUA_MINSTACK - 1) {
                lua_concat(ls, lua_gettop(ls) - 3);
            }
            lua_pushinteger(ls, offset);
            lua_replace(ls, 3);
            enable_rx_irq(inst);
            return -1;
        }
        luaL_addchar(&buf, (char)uart_get_hw(inst)->dr);
        ++offset;
    }
    luaL_pushresult(&buf);
    lua_concat(ls, lua_gettop(ls) - 3);
    return 1;
}

static int UART_read_blocking(lua_State* ls) {
    uart_inst_t* inst = mlua_check_UART(ls, 1);
    lua_Integer len = luaL_checkinteger(ls, 2);
    if (len <= 0) {
        lua_pushliteral(ls, "");
        return 1;
    }
    MLuaEvent* event = &uart_state[uart_get_index(inst)].rx_event;
    if (mlua_event_can_wait(ls, event, 0)) {
        lua_settop(ls, 2);
        lua_pushinteger(ls, 0);  // offset
        return mlua_event_wait(ls, event, 0, &read_loop, 0);
    }
    luaL_Buffer buf;
    uint8_t* dst = (uint8_t*)luaL_buffinitsize(ls, &buf, len);
    uart_read_blocking(inst, dst, len);
    luaL_pushresultsize(&buf, len);
    return 1;
}

static int getc_loop(lua_State* ls, bool timeout) {
    uart_inst_t* inst = to_UART(ls, 1);
    if (!uart_is_readable(inst)) return -1;
    lua_pushinteger(ls, uart_getc(inst));
    return 1;
}

static int UART_getc(lua_State* ls) {
    uart_inst_t* inst = mlua_check_UART(ls, 1);
    MLuaEvent* event = &uart_state[uart_get_index(inst)].rx_event;
    if (mlua_event_can_wait(ls, event, 0)) {
        return mlua_event_wait(ls, event, 0, &getc_loop, 0);
    }
    lua_pushinteger(ls, uart_getc(inst));
    return 1;
}

static int is_readable_loop(lua_State* ls, bool timeout) {
    uart_inst_t* inst = to_UART(ls, 1);
    bool readable = uart_is_readable(inst);
    if (!(readable || timeout)) return -1;
    return lua_pushboolean(ls, readable), 1;
}

static int UART_is_readable_within_us(lua_State* ls) {
    uart_inst_t* inst = mlua_check_UART(ls, 1);
    uint32_t timeout = luaL_checkinteger(ls, 2);
    MLuaEvent* event = &uart_state[uart_get_index(inst)].rx_event;
    if (mlua_event_can_wait(ls, event, 0)) {
        lua_settop(ls, 1);
        mlua_push_deadline(ls, timeout);
        return mlua_event_wait(ls, event, 0, &is_readable_loop, 2);
    }
    lua_pushboolean(ls, uart_is_readable_within_us(inst, timeout));
    return 1;
}

static int UART_enable_loopback(lua_State* ls) {
    uart_inst_t* inst = mlua_check_UART(ls, 1);
    if (mlua_to_cbool(ls, 2)) {
        hw_set_bits(&uart_get_hw(inst)->cr, UART_UARTCR_LBE_BITS);
        // Switching the loopback mode on causes an error to be pushed to the
        // FIFO. Drain the FIFO to get rid of them.
        while (uart_is_readable(inst)) (void)uart_get_hw(inst)->dr;
    } else {
        hw_clear_bits(&uart_get_hw(inst)->cr, UART_UARTCR_LBE_BITS);
    }
    return 0;
}

MLUA_FUNC_R1(UART_, uart_, get_index, lua_pushinteger, mlua_check_UART)
MLUA_FUNC_R2(UART_, uart_, init, lua_pushinteger, mlua_check_UART,
             luaL_checkinteger)
MLUA_FUNC_R2(UART_, uart_, set_baudrate, lua_pushinteger, mlua_check_UART,
             luaL_checkinteger)
MLUA_FUNC_V3(UART_, uart_, set_hw_flow, mlua_check_UART, mlua_to_cbool,
             mlua_to_cbool)
MLUA_FUNC_V4(UART_, uart_, set_format, mlua_check_UART, luaL_checkinteger,
             luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_V3(UART_, uart_, set_irq_enables, mlua_check_UART, mlua_to_cbool,
             mlua_to_cbool)
MLUA_FUNC_R1(UART_, uart_, is_enabled, lua_pushboolean, mlua_check_UART)
MLUA_FUNC_V2(UART_, uart_, set_fifo_enabled, mlua_check_UART, mlua_to_cbool)
MLUA_FUNC_R1(UART_, uart_, is_writable, lua_pushboolean, mlua_check_UART)
MLUA_FUNC_R1(UART_, uart_, is_readable, lua_pushboolean, mlua_check_UART)
MLUA_FUNC_V2(UART_, uart_, putc_raw, mlua_check_UART, luaL_checkinteger)
MLUA_FUNC_V2(UART_, uart_, putc, mlua_check_UART, luaL_checkinteger)
MLUA_FUNC_V2(UART_, uart_, puts, mlua_check_UART, luaL_checkstring)
MLUA_FUNC_V2(UART_, uart_, set_break, mlua_check_UART, mlua_to_cbool)
MLUA_FUNC_V2(UART_, uart_, set_translate_crlf, mlua_check_UART, mlua_to_cbool)
MLUA_FUNC_R2(UART_, uart_, get_dreq, lua_pushinteger, mlua_check_UART,
             mlua_to_cbool)

MLUA_SYMBOLS(UART_syms) = {
    MLUA_SYM_F(get_index, UART_),
    MLUA_SYM_F(regs, UART_),
    MLUA_SYM_F(init, UART_),
    MLUA_SYM_F(deinit, UART_),
    MLUA_SYM_F(set_baudrate, UART_),
    MLUA_SYM_F(set_hw_flow, UART_),
    MLUA_SYM_F(set_format, UART_),
    MLUA_SYM_F(set_irq_enables, UART_),
    MLUA_SYM_F(is_enabled, UART_),
    MLUA_SYM_F(set_fifo_enabled, UART_),
    MLUA_SYM_F(is_writable, UART_),
    MLUA_SYM_F(is_tx_busy, UART_),
    MLUA_SYM_F(tx_wait_blocking, UART_),
    MLUA_SYM_F(is_readable, UART_),
    MLUA_SYM_F(write_blocking, UART_),
    MLUA_SYM_F(read_blocking, UART_),
    MLUA_SYM_F(putc_raw, UART_),
    MLUA_SYM_F(putc, UART_),
    MLUA_SYM_F(puts, UART_),
    MLUA_SYM_F(getc, UART_),
    MLUA_SYM_F(set_break, UART_),
    MLUA_SYM_F(set_translate_crlf, UART_),
    MLUA_SYM_F(is_readable_within_us, UART_),
    MLUA_SYM_F(get_dreq, UART_),
    MLUA_SYM_F(enable_loopback, UART_),
    MLUA_SYM_F_THREAD(enable_irq, UART_),
};

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(NUM, integer, NUM_UARTS),
    MLUA_SYM_V(_default, boolean, false),
    MLUA_SYM_V(ENABLE_CRLF_SUPPORT, boolean, PICO_UART_ENABLE_CRLF_SUPPORT),
    MLUA_SYM_V(DEFAULT_CRLF, boolean, PICO_UART_DEFAULT_CRLF),
    MLUA_SYM_V(DEFAULT_BAUD_RATE, integer, PICO_DEFAULT_UART_BAUD_RATE),
    MLUA_SYM_V(PARITY_NONE, integer, UART_PARITY_NONE),
    MLUA_SYM_V(PARITY_EVEN, integer, UART_PARITY_EVEN),
    MLUA_SYM_V(PARITY_ODD, integer, UART_PARITY_ODD),
    // uart_default_tx_wait_blocking: Use default.tx_wait_blocking()
};

MLUA_OPEN_MODULE(hardware.uart) {
    mlua_thread_require(ls);
    mlua_require(ls, "mlua.int64", false);

    // Create the module.
    mlua_new_module(ls, NUM_UARTS, module_syms);
    int mod_index = lua_gettop(ls);

    // Create the UART class.
    mlua_new_class(ls, mlua_UART_name, UART_syms, mlua_nosyms);
    lua_pop(ls, 1);

    // Create UART instances.
    for (uint i = 0; i < NUM_UARTS; ++i) {
        uart_inst_t* inst = uart_get_instance(i);
        *new_UART(ls) = inst;
#ifdef uart_default
        if (inst == uart_default) {
            lua_pushvalue(ls, -1);
            lua_setfield(ls, mod_index, "default");
        }
#endif
        lua_seti(ls, mod_index, i);
    }
    return 1;
}
