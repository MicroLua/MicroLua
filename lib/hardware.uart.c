#include <stdint.h>

#include "hardware/address_mapped.h"
#include "hardware/irq.h"
#include "hardware/regs/intctrl.h"
#include "hardware/uart.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/util.h"

static char const Uart_name[] = "hardware.uart.Uart";

static uart_inst_t** new_Uart(lua_State* ls) {
    uart_inst_t** v = lua_newuserdatauv(ls, sizeof(uart_inst_t*), 0);
    luaL_getmetatable(ls, Uart_name);
    lua_setmetatable(ls, -2);
    return v;
}

static inline uart_inst_t* check_Uart(lua_State* ls, int arg) {
    return *((uart_inst_t**)luaL_checkudata(ls, arg, Uart_name));
}

static inline uart_inst_t* to_Uart(lua_State* ls, int arg) {
    return *((uart_inst_t**)lua_touserdata(ls, arg));
}

static int Uart_tx_wait_blocking_1(lua_State* ls, int status, lua_KContext ctx);

static int Uart_tx_wait_blocking(lua_State* ls) {
    uart_inst_t* u = check_Uart(ls, 1);
    if (mlua_yield_enabled()) {
        if ((uart_get_hw(u)->fr & UART_UARTFR_BUSY_BITS) == 0) return 0;
        // Busy loop, same as uart_tx_wait_blocking.
        return mlua_event_yield(ls, &Uart_tx_wait_blocking_1, 0, 0);
    }
    uart_tx_wait_blocking(u);
    return 0;
}

#if LIB_MLUA_MOD_MLUA_EVENT

static int Uart_tx_wait_blocking_1(lua_State* ls, int status,
                                   lua_KContext ctx) {
    uart_inst_t* u = to_Uart(ls, 1);
    if ((uart_get_hw(u)->fr & UART_UARTFR_BUSY_BITS) == 0) return 0;
    return mlua_event_yield(ls, &Uart_tx_wait_blocking_1, 0, 0);
}

#endif  // LIB_MLUA_MOD_MLUA_EVENT

#if LIB_MLUA_MOD_MLUA_EVENT

typedef struct UartState {
    MLuaEvent rx_event;
    MLuaEvent tx_event;
} UartState;

static UartState uart_state[NUM_UARTS];

static void __time_critical_func(handle_irq)(void) {
    uint num = __get_current_exception() - VTABLE_FIRST_IRQ - UART0_IRQ;
    uart_hw_t* hu = uart_get_hw(uart_get_instance(num));
    uint32_t mis = hu->mis;
    hw_clear_bits(&hu->imsc, mis & (
        UART_UARTMIS_RXMIS_BITS | UART_UARTMIS_RTMIS_BITS
        | UART_UARTMIS_TXMIS_BITS));
    UartState* state = &uart_state[num];
    if (mis & (UART_UARTMIS_RXMIS_BITS | UART_UARTMIS_RTMIS_BITS)) {
        mlua_event_set(state->rx_event);
    }
    if (mis & UART_UARTMIS_TXMIS_BITS) {
        mlua_event_set(state->tx_event);
    }
}

static int Uart_enable_irq(lua_State* ls) {
    uart_inst_t* u = to_Uart(ls, 1);
    uint num = uart_get_index(u);
    uint irq = UART0_IRQ + num;
    UartState* state = &uart_state[num];
    lua_Integer priority = -1;
    if (!mlua_event_enable_irq_arg(ls, 2, &priority)) {  // Disable IRQ
        mlua_event_remove_irq_handler(irq, &handle_irq);
        mlua_event_unclaim(ls, &state->rx_event);
        mlua_event_unclaim(ls, &state->tx_event);
        return 0;
    }
    char const* err = mlua_event_claim(&state->rx_event);
    if (err != NULL) return luaL_error(ls, "UART%d Rx: %s", num, err);
    err = mlua_event_claim(&state->tx_event);
    if (err != NULL) {
        mlua_event_unclaim(ls, &state->rx_event);
        return luaL_error(ls, "UART%d Tx: %s", num, err);
    }
    hw_write_masked(&uart_get_hw(u)->ifls,  // Thresholds to 1/2 FIFO capacity
                    (2 << UART_UARTIFLS_RXIFLSEL_LSB)
                    | (2 << UART_UARTIFLS_TXIFLSEL_LSB),
                    UART_UARTIFLS_RXIFLSEL_BITS | UART_UARTMIS_TXMIS_BITS);
    mlua_event_set_irq_handler(irq, &handle_irq, priority);
    return 0;
}

#endif  // LIB_MLUA_MOD_MLUA_EVENT

static int Uart_deinit(lua_State* ls) {
    uart_inst_t* u = check_Uart(ls, 1);
#if LIB_MLUA_MOD_MLUA_EVENT
    lua_pushcfunction(ls, &Uart_enable_irq);
    lua_pushvalue(ls, 1);
    lua_pushboolean(ls, false);
    lua_call(ls, 2, 0);
#endif
    uart_deinit(u);
    return 0;
}

static inline void enable_tx_irq(uart_inst_t* u) {
    hw_set_bits(&uart_get_hw(u)->imsc, UART_UARTIMSC_TXIM_BITS);
}

static int try_write(lua_State* ls) {
    uart_inst_t* u = to_Uart(ls, 1);
    size_t len;
    uint8_t const* src = (uint8_t const*)lua_tolstring(ls, 2, &len);
    size_t offset = lua_tointeger(ls, 3);
    lua_pop(ls, 1);
    while (offset < len) {
        if (!uart_is_writable(u)) {
            lua_pushinteger(ls, offset);
            enable_tx_irq(u);
            return -1;
        }
        uart_get_hw(u)->dr = src[offset];
        ++offset;
    }
    return 0;
}

static int Uart_write_blocking(lua_State* ls) {
    uart_inst_t* u = check_Uart(ls, 1);
    size_t len;
    uint8_t const* src = (uint8_t const*)luaL_checklstring(ls, 2, &len);
    if (len == 0) return 0;
    if (mlua_yield_enabled()) {
        lua_settop(ls, 2);
        lua_pushinteger(ls, 0);  // offset = 0
        return mlua_event_wait(ls, uart_state[uart_get_index(u)].tx_event,
                               &try_write, 0);
    }
    uart_write_blocking(u, src, len);
    return 0;
}

static inline void enable_rx_irq(uart_inst_t* u) {
    hw_set_bits(&uart_get_hw(u)->imsc,
                UART_UARTIMSC_RXIM_BITS | UART_UARTIMSC_RTIM_BITS);
}

static int try_read(lua_State* ls) {
    uart_inst_t* u = to_Uart(ls, 1);
    size_t len = lua_tointeger(ls, 2);
    size_t offset = lua_tointeger(ls, 3);
    luaL_Buffer buf;
    luaL_buffinit(ls, &buf);
    while (offset < len) {
        if (!uart_is_readable(u)) {
            luaL_pushresult(&buf);
            lua_pushinteger(ls, offset);
            lua_replace(ls, 3);
            enable_rx_irq(u);
            return -1;
        }
        luaL_addchar(&buf, (char)uart_get_hw(u)->dr);
        ++offset;
    }
    luaL_pushresult(&buf);
    lua_concat(ls, lua_gettop(ls) - 3);
    return 1;
}

static int Uart_read_blocking(lua_State* ls) {
    uart_inst_t* u = check_Uart(ls, 1);
    lua_Integer len = luaL_checkinteger(ls, 2);
    if (len <= 0) {
        lua_pushliteral(ls, "");
        return 1;
    }
    if (mlua_yield_enabled()) {
        lua_settop(ls, 2);
        lua_pushinteger(ls, 0);  // offset = 0
        return mlua_event_wait(ls, uart_state[uart_get_index(u)].rx_event,
                               &try_read, 0);
    }
    luaL_Buffer buf;
    uint8_t* dst = (uint8_t*)luaL_buffinitsize(ls, &buf, len);
    uart_read_blocking(u, dst, len);
    luaL_pushresultsize(&buf, len);
    return 1;
}

static int Uart_enable_loopback(lua_State* ls) {
    uart_inst_t* u = check_Uart(ls, 1);
    if (mlua_to_cbool(ls, 2)) {
        hw_set_bits(&uart_get_hw(u)->cr, UART_UARTCR_LBE_BITS);
        // Switching the loopback mode on causes an error to be pushed to the
        // FIFO. Drain the FIFO to get rid of them.
        while (uart_is_readable(u)) (void)uart_get_hw(u)->dr;
    } else {
        hw_clear_bits(&uart_get_hw(u)->cr, UART_UARTCR_LBE_BITS);
    }
    return 0;
}

MLUA_FUNC_1_1(Uart_, uart_, get_index, lua_pushinteger, check_Uart)
MLUA_FUNC_1_2(Uart_, uart_, init, lua_pushinteger, check_Uart,
              luaL_checkinteger)
MLUA_FUNC_1_2(Uart_, uart_, set_baudrate, lua_pushinteger, check_Uart,
              luaL_checkinteger)
MLUA_FUNC_0_3(Uart_, uart_, set_hw_flow, check_Uart, mlua_to_cbool,
              mlua_to_cbool)
MLUA_FUNC_0_4(Uart_, uart_, set_format, check_Uart, luaL_checkinteger,
              luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_0_3(Uart_, uart_, set_irq_enables, check_Uart, mlua_to_cbool,
              mlua_to_cbool)
MLUA_FUNC_1_1(Uart_, uart_, is_enabled, lua_pushboolean, check_Uart)
MLUA_FUNC_0_2(Uart_, uart_, set_fifo_enabled, check_Uart, mlua_to_cbool)
MLUA_FUNC_1_1(Uart_, uart_, is_writable, lua_pushboolean, check_Uart)
MLUA_FUNC_1_1(Uart_, uart_, is_readable, lua_pushboolean, check_Uart)
MLUA_FUNC_0_2(Uart_, uart_, set_break, check_Uart, mlua_to_cbool)
MLUA_FUNC_0_2(Uart_, uart_, set_translate_crlf, check_Uart, mlua_to_cbool)
MLUA_FUNC_1_2(Uart_, uart_, get_dreq, lua_pushinteger, check_Uart,
              mlua_to_cbool)

MLUA_FUNC_0_0(mod_, uart_, default_tx_wait_blocking)

static MLuaReg const Uart_regs[] = {
#define MLUA_SYM(n) MLUA_REG(function, n, Uart_ ## n)
    MLUA_SYM(get_index),
    MLUA_SYM(init),
    MLUA_SYM(deinit),
    MLUA_SYM(set_baudrate),
    MLUA_SYM(set_hw_flow),
    MLUA_SYM(set_format),
    MLUA_SYM(set_irq_enables),
    MLUA_SYM(is_enabled),
    MLUA_SYM(set_fifo_enabled),
    MLUA_SYM(is_writable),
    // MLUA_SYM(wait_writable),
    // MLUA_SYM(tx_is_busy),
    MLUA_SYM(tx_wait_blocking),
    MLUA_SYM(is_readable),
    // MLUA_SYM(wait_readable),
    MLUA_SYM(write_blocking),
    MLUA_SYM(read_blocking),
    // MLUA_SYM(putc_raw),
    // MLUA_SYM(putc),
    // MLUA_SYM(puts),
    // MLUA_SYM(getc),
    MLUA_SYM(set_break),
    MLUA_SYM(set_translate_crlf),
    // MLUA_SYM(is_readable_within_us),
    MLUA_SYM(get_dreq),
    MLUA_SYM(enable_loopback),
#if LIB_MLUA_MOD_MLUA_EVENT
    MLUA_SYM(enable_irq),
#endif
#undef MLUA_SYM
};

static MLuaReg const module_regs[] = {
#define MLUA_SYM(n) MLUA_REG(integer, n, UART_ ## n)
    MLUA_SYM(PARITY_NONE),
    MLUA_SYM(PARITY_EVEN),
    MLUA_SYM(PARITY_ODD),
#undef MLUA_SYM
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(default_tx_wait_blocking),
#undef MLUA_SYM
};

#if LIB_MLUA_MOD_MLUA_EVENT

static __attribute__((constructor)) void init(void) {
    for (uint i = 0; i < NUM_UARTS; ++i) {
        UartState* state = &uart_state[i];
        state->rx_event = MLUA_EVENT_UNSET;
        state->tx_event = MLUA_EVENT_UNSET;
    }
}

#endif  // LIB_MLUA_MOD_MLUA_EVENT

int luaopen_hardware_uart(lua_State* ls) {
    mlua_event_require(ls);

    // Create the module.
    lua_createtable(ls, 2, MLUA_SIZE(module_regs) + 1);
    mlua_set_fields(ls, module_regs);
    int mod_index = lua_gettop(ls);

    // Create the Uart class.
    mlua_new_class(ls, Uart_name, Uart_regs);
    lua_pop(ls, 1);

    // Create Uart instances.
    for (uint i = 0; i < NUM_UARTS; ++i) {
        uart_inst_t* u = uart_get_instance(i);
        *new_Uart(ls) = u;
#ifdef uart_default
        if (u == uart_default) {
            lua_pushvalue(ls, -1);
            lua_setfield(ls, mod_index, "default");
        }
#endif
        lua_seti(ls, mod_index, i);
    }
    return 1;
}
