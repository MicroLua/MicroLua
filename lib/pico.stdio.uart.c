#include "pico/stdio_uart.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/hardware.uart.h"
#include "mlua/util.h"

#include "hardware/gpio.h"
#include "pico/stdio.h"
#include "pico/stdio_uart.h"

MLUA_FUNC_0_0(mod_, stdio_uart_, init)
MLUA_FUNC_0_0(mod_init_stdout, stdout_uart_init,)
MLUA_FUNC_0_0(mod_init_stdin, stdin_uart_init,)
MLUA_FUNC_0_4(mod_, stdio_uart_, init_full, mlua_check_Uart, luaL_checkinteger,
              luaL_checkinteger, luaL_checkinteger)

static MLuaSym const module_syms[] = {
    MLUA_SYM_V(driver, lightuserdata, &stdio_uart),

    MLUA_SYM_F(init, mod_),
    MLUA_SYM_F(init_stdout, mod_),
    MLUA_SYM_F(init_stdin, mod_),
    MLUA_SYM_F(init_full, mod_),
};

int luaopen_pico_stdio_uart(lua_State* ls) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
