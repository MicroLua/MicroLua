#include "pico.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(integer, n, PICO_ ## n)
    X(OK),
    X(ERROR_NONE),
    X(ERROR_TIMEOUT),
    X(ERROR_GENERIC),
    X(ERROR_NO_DATA),
    X(ERROR_NOT_PERMITTED),
    X(ERROR_INVALID_ARG),
    X(ERROR_IO),
    X(ERROR_BADAUTH),
    X(ERROR_CONNECT_FAILED),
    X(SDK_VERSION_MAJOR),
    X(SDK_VERSION_MINOR),
    X(SDK_VERSION_REVISION),
#ifdef PICO_DEFAULT_UART
    X(DEFAULT_UART),
#endif
#ifdef PICO_DEFAULT_UART_TX_PIN
    X(DEFAULT_UART_TX_PIN),
#endif
#ifdef PICO_DEFAULT_UART_RX_PIN
    X(DEFAULT_UART_RX_PIN),
#endif
#ifdef PICO_DEFAULT_LED_PIN
    X(DEFAULT_LED_PIN),
#endif
#ifdef PICO_DEFAULT_I2C
    X(DEFAULT_I2C),
#endif
#ifdef PICO_DEFAULT_I2C_SDA_PIN
    X(DEFAULT_I2C_SDA_PIN),
#endif
#ifdef PICO_DEFAULT_I2C_SCL_PIN
    X(DEFAULT_I2C_SCL_PIN),
#endif
#ifdef PICO_DEFAULT_SCK_PIN
    X(DEFAULT_SCK_PIN),
#endif
#ifdef PICO_DEFAULT_TX_PIN
    X(DEFAULT_TX_PIN),
#endif
#ifdef PICO_DEFAULT_SPI_RX_PIN
    X(DEFAULT_SPI_RX_PIN),
#endif
#ifdef PICO_DEFAULT_SPI_CSN_PIN
    X(DEFAULT_SPI_CSN_PIN),
#endif
#ifdef PICO_FLASH_SPI_CLKDIV
    X(FLASH_SPI_CLKDIV),
#endif
#ifdef PICO_FLASH_SIZE_BYTES
    X(FLASH_SIZE_BYTES),
#endif
#ifdef PICO_SMPS_MODE_PIN
    X(SMPS_MODE_PIN),
#endif
#undef X
#define X(n) MLUA_REG(string, n, PICO_ ## n)
    X(SDK_VERSION_STRING),
#undef X
#define X(n) {.name=#n, .push=mlua_reg_push_integer, .integer=n}
#ifdef CYW43_WL_GPIO_COUNT
    X(CYW43_WL_GPIO_COUNT),
#endif
#ifdef CYW43_WL_GPIO_LED_PIN
    X(CYW43_WL_GPIO_LED_PIN),
#endif
#undef X
    {NULL},
};

int luaopen_pico(lua_State* ls) {
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
