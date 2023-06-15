#include <stdint.h>
#include <stdio.h>

#include "hardware/uart.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

char const* const Uart_name = "hardware.uart.Uart";

static uart_inst_t** new_Uart(lua_State* ls) {
    uart_inst_t** v = lua_newuserdatauv(ls, sizeof(uart_inst_t*), 0);
    luaL_getmetatable(ls, Uart_name);
    lua_setmetatable(ls, -2);
    return v;
}

static inline uart_inst_t* check_Uart(lua_State* ls, int arg) {
    return *((uart_inst_t**)luaL_checkudata(ls, arg, Uart_name));
}

static int Uart_write_blocking(lua_State* ls) {
    uart_inst_t* u = check_Uart(ls, 1);
    size_t len;
    uint8_t const* src = (uint8_t const*)luaL_checklstring(ls, 2, &len);
    uart_write_blocking(u, src, len);
    return 0;
}

static int Uart_read_blocking(lua_State* ls) {
    uart_inst_t* u = check_Uart(ls, 1);
    lua_Integer len = luaL_checkinteger(ls, 2);
    luaL_Buffer buf;
    uint8_t* dst = (uint8_t*)luaL_buffinitsize(ls, &buf, len);
    uart_read_blocking(u, dst, len);
    luaL_pushresultsize(&buf, len);
    return 1;
}


MLUA_FUNC_1_1(Uart_, uart_, get_index, lua_pushinteger, check_Uart)
MLUA_FUNC_1_2(Uart_, uart_, init, lua_pushinteger, check_Uart,
              luaL_checkinteger)
MLUA_FUNC_0_1(Uart_, uart_, deinit, check_Uart)
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
MLUA_FUNC_0_1(Uart_, uart_, tx_wait_blocking, check_Uart)
MLUA_FUNC_1_1(Uart_, uart_, is_readable, lua_pushboolean, check_Uart)
MLUA_FUNC_0_2(Uart_, uart_, putc_raw, check_Uart, luaL_checkinteger)
MLUA_FUNC_0_2(Uart_, uart_, putc, check_Uart, luaL_checkinteger)
MLUA_FUNC_0_2(Uart_, uart_, puts, check_Uart, luaL_checkstring)
MLUA_FUNC_1_1(Uart_, uart_, getc, lua_pushinteger, check_Uart)
MLUA_FUNC_0_2(Uart_, uart_, set_break, check_Uart, mlua_to_cbool)
MLUA_FUNC_0_2(Uart_, uart_, set_translate_crlf, check_Uart, mlua_to_cbool)
MLUA_FUNC_1_2(Uart_, uart_, is_readable_within_us, lua_pushboolean, check_Uart,
              luaL_checkinteger)
MLUA_FUNC_1_2(Uart_, uart_, get_dreq, lua_pushinteger, check_Uart,
              mlua_to_cbool)

MLUA_FUNC_0_0(mod_, uart_, default_tx_wait_blocking)

static MLuaReg const Uart_regs[] = {
#define MLUA_SYM(n) MLUA_REG(integer, n, UART_ ## n)
    MLUA_SYM(PARITY_NONE),
    MLUA_SYM(PARITY_EVEN),
    MLUA_SYM(PARITY_ODD),
#undef MLUA_SYM
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
    // MLUA_SYM(is_tx_busy),
    MLUA_SYM(tx_wait_blocking),
    MLUA_SYM(is_readable),
    MLUA_SYM(write_blocking),
    MLUA_SYM(read_blocking),
    MLUA_SYM(putc_raw),
    MLUA_SYM(putc),
    MLUA_SYM(puts),
    MLUA_SYM(getc),
    MLUA_SYM(set_break),
    MLUA_SYM(set_translate_crlf),
    MLUA_SYM(is_readable_within_us),
    MLUA_SYM(get_dreq),
#undef MLUA_SYM
    {NULL},
};

static MLuaReg const module_regs[] = {
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(default_tx_wait_blocking),
#undef MLUA_SYM
    {NULL},
};

int luaopen_hardware_uart(lua_State* ls) {
    // Create the Uart class.
    mlua_new_class(ls, Uart_name, Uart_regs);
    lua_pop(ls, 1);

    // Create the module.
    mlua_newlib(ls, module_regs, 2 + NUM_UARTS, 0);
    int mod_index = lua_gettop(ls);
    lua_createtable(ls, NUM_UARTS, 0);
    int uart_index = lua_gettop(ls);
    lua_pushvalue(ls, -1);
    lua_setfield(ls, mod_index, "uart");

    // Create Uart instances.
    char name[4 + (NUM_UARTS < 10 ? 1 : 2) + 1];
    for (uint i = 0; i < NUM_UARTS; ++i) {
        uart_inst_t* u = uart_get_instance(i);
        *new_Uart(ls) = u;
#ifdef uart_default
        if (u == uart_default) {
            lua_pushvalue(ls, -1);
            lua_setfield(ls, mod_index, "default");
        }
#endif
        lua_pushvalue(ls, -1);
        lua_seti(ls, uart_index, i);
        snprintf(name, sizeof(name), "uart%d", i);
        lua_setfield(ls, mod_index, name);
    }
    lua_pop(ls, 1);  // Remove uart
    return 1;
}
