#include "pico.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

char const __flash_binary_start;
char const __flash_binary_end;

static MLuaReg const module_regs[] = {
#define MLUA_SYM(n, v) MLUA_REG(integer, n, v)
    MLUA_SYM(flash_binary_start, (lua_Integer)&__flash_binary_start),
    MLUA_SYM(flash_binary_end, (lua_Integer)&__flash_binary_end),
#undef MLUA_SYM
#define MLUA_SYM(n) MLUA_REG(integer, n, PICO_ ## n)
    MLUA_SYM(OK),
    MLUA_SYM(ERROR_NONE),
    MLUA_SYM(ERROR_TIMEOUT),
    MLUA_SYM(ERROR_GENERIC),
    MLUA_SYM(ERROR_NO_DATA),
    MLUA_SYM(ERROR_NOT_PERMITTED),
    MLUA_SYM(ERROR_INVALID_ARG),
    MLUA_SYM(ERROR_IO),
    MLUA_SYM(ERROR_BADAUTH),
    MLUA_SYM(ERROR_CONNECT_FAILED),
    MLUA_SYM(SDK_VERSION_MAJOR),
    MLUA_SYM(SDK_VERSION_MINOR),
    MLUA_SYM(SDK_VERSION_REVISION),
#ifdef PICO_DEFAULT_UART
    MLUA_SYM(DEFAULT_UART),
#endif
#ifdef PICO_DEFAULT_UART_TX_PIN
    MLUA_SYM(DEFAULT_UART_TX_PIN),
#endif
#ifdef PICO_DEFAULT_UART_RX_PIN
    MLUA_SYM(DEFAULT_UART_RX_PIN),
#endif
#ifdef PICO_DEFAULT_LED_PIN
    MLUA_SYM(DEFAULT_LED_PIN),
#endif
#ifdef PICO_DEFAULT_I2C
    MLUA_SYM(DEFAULT_I2C),
#endif
#ifdef PICO_DEFAULT_I2C_SDA_PIN
    MLUA_SYM(DEFAULT_I2C_SDA_PIN),
#endif
#ifdef PICO_DEFAULT_I2C_SCL_PIN
    MLUA_SYM(DEFAULT_I2C_SCL_PIN),
#endif
#ifdef PICO_DEFAULT_SCK_PIN
    MLUA_SYM(DEFAULT_SCK_PIN),
#endif
#ifdef PICO_DEFAULT_TX_PIN
    MLUA_SYM(DEFAULT_TX_PIN),
#endif
#ifdef PICO_DEFAULT_SPI_RX_PIN
    MLUA_SYM(DEFAULT_SPI_RX_PIN),
#endif
#ifdef PICO_DEFAULT_SPI_CSN_PIN
    MLUA_SYM(DEFAULT_SPI_CSN_PIN),
#endif
#ifdef PICO_FLASH_SPI_CLKDIV
    MLUA_SYM(FLASH_SPI_CLKDIV),
#endif
#ifdef PICO_FLASH_SIZE_BYTES
    MLUA_SYM(FLASH_SIZE_BYTES),
#endif
#ifdef PICO_SMPS_MODE_PIN
    MLUA_SYM(SMPS_MODE_PIN),
#endif
#undef MLUA_SYM
#define MLUA_SYM(n) MLUA_REG(string, n, PICO_ ## n)
    MLUA_SYM(SDK_VERSION_STRING),
#undef MLUA_SYM
#define MLUA_SYM(n) {.name=#n, .push=mlua_reg_push_integer, .integer=n}
#ifdef CYW43_WL_GPIO_COUNT
    MLUA_SYM(CYW43_WL_GPIO_COUNT),
#endif
#ifdef CYW43_WL_GPIO_LED_PIN
    MLUA_SYM(CYW43_WL_GPIO_LED_PIN),
#endif
#undef MLUA_SYM
};

int luaopen_pico(lua_State* ls) {
    mlua_new_table(ls, module_regs);
    return 1;
}
