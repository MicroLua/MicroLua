#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

MLUA_FUNC_1_0(mod_,, rp2040_chip_version, lua_pushinteger)
MLUA_FUNC_1_0(mod_,, rp2040_rom_version, lua_pushinteger)
MLUA_FUNC_0_1(mod_,, busy_wait_at_least_cycles, luaL_checkinteger)
MLUA_FUNC_1_0(mod_,, get_core_num, lua_pushinteger)

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(function, n, mod_ ## n)
    X(rp2040_chip_version),
    X(rp2040_rom_version),
    X(busy_wait_at_least_cycles),
    X(get_core_num),
#undef X
#define X(n) {.name=#n, .push=mlua_reg_push_integer, .integer=n}
    X(NUM_CORES),
    X(NUM_DMA_CHANNELS),
    X(NUM_DMA_TIMERS),
    X(NUM_IRQS),
    X(NUM_USER_IRQS),
    X(NUM_PIOS),
    X(NUM_PIO_STATE_MACHINES),
    X(NUM_PWM_SLICES),
    X(NUM_SPIN_LOCKS),
    X(NUM_UARTS),
    X(NUM_I2CS),
    X(NUM_SPIS),
    X(NUM_TIMERS),
    X(NUM_ADC_CHANNELS),
    X(NUM_BANK0_GPIOS),
    X(NUM_QSPI_GPIOS),
    X(PIO_INSTRUCTION_COUNT),
    X(FIRST_USER_IRQ),
#undef X
#define X(n) MLUA_REG(integer, n, PICO_ ## n)
#ifdef PICO_RP2040
    X(RP2040),
#endif
#undef X
    {NULL},
};

int luaopen_pico_platform(lua_State* ls) {
    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
