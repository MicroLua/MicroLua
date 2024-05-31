// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "pico/stdio_uart.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/hardware.uart.h"
#include "mlua/module.h"

MLUA_FUNC_V0(mod_, stdio_uart_, init)
MLUA_FUNC_V0(mod_init_stdout, stdout_uart_init,)
MLUA_FUNC_V0(mod_init_stdin, stdin_uart_init,)
MLUA_FUNC_V4(mod_, stdio_uart_, init_full, mlua_check_UART, luaL_checkinteger,
             luaL_checkinteger, luaL_checkinteger)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(driver, lightuserdata, &stdio_uart),

    MLUA_SYM_F(init, mod_),
    MLUA_SYM_F(init_stdout, mod_),
    MLUA_SYM_F(init_stdin, mod_),
    MLUA_SYM_F(init_full, mod_),
};

// When true, initialize UART-based stdin.
#ifndef MLUA_STDIO_INIT_UART_IN
#define MLUA_STDIO_INIT_UART_IN 0
#endif

// When true, initialize UART-based stdout.
#ifndef MLUA_STDIO_INIT_UART_OUT
#define MLUA_STDIO_INIT_UART_OUT 0
#endif

static __attribute__((constructor)) void init(void) {
#if MLUA_STDIO_INIT_UART_IN && MLUA_STDIO_INIT_UART_OUT
    stdio_uart_init();
#elif MLUA_STDIO_INIT_UART_IN
    stdin_uart_init();
#elif MLUA_STDIO_INIT_UART_OUT
    stdout_uart_init();
#endif
}

MLUA_OPEN_MODULE(pico.stdio.uart) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
