#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

MLUA_FUNC_1_0(mod_,, rp2040_chip_version, lua_pushinteger)
MLUA_FUNC_1_0(mod_,, rp2040_rom_version, lua_pushinteger)
MLUA_FUNC_0_1(mod_,, busy_wait_at_least_cycles, luaL_checkinteger)
MLUA_FUNC_1_0(mod_,, get_core_num, lua_pushinteger)

static MLuaReg const module_regs[] = {
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(rp2040_chip_version),
    MLUA_SYM(rp2040_rom_version),
    MLUA_SYM(busy_wait_at_least_cycles),
    MLUA_SYM(get_core_num),
#undef MLUA_SYM
#define MLUA_SYM(n) {.name=#n, .push=mlua_reg_push_integer, .integer=n}
    MLUA_SYM(NUM_CORES),
    MLUA_SYM(NUM_DMA_CHANNELS),
    MLUA_SYM(NUM_DMA_TIMERS),
    MLUA_SYM(NUM_IRQS),
    MLUA_SYM(NUM_USER_IRQS),
    MLUA_SYM(NUM_PIOS),
    MLUA_SYM(NUM_PIO_STATE_MACHINES),
    MLUA_SYM(NUM_PWM_SLICES),
    MLUA_SYM(NUM_SPIN_LOCKS),
    MLUA_SYM(NUM_UARTS),
    MLUA_SYM(NUM_I2CS),
    MLUA_SYM(NUM_SPIS),
    MLUA_SYM(NUM_TIMERS),
    MLUA_SYM(NUM_ADC_CHANNELS),
    MLUA_SYM(NUM_BANK0_GPIOS),
    MLUA_SYM(NUM_QSPI_GPIOS),
    MLUA_SYM(PIO_INSTRUCTION_COUNT),
    MLUA_SYM(FIRST_USER_IRQ),
#undef MLUA_SYM
#define MLUA_SYM(n) MLUA_REG(integer, n, PICO_ ## n)
#ifdef PICO_RP2040
    MLUA_SYM(RP2040),
#endif
#undef MLUA_SYM
};

int luaopen_pico_platform(lua_State* ls) {
    // Create the module.
    mlua_new_table(ls, module_regs);
    return 1;
}
