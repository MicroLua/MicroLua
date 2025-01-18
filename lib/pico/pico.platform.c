// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "pico.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

MLUA_FUNC_R0(mod_,, rp2040_chip_version, lua_pushinteger)
MLUA_FUNC_R0(mod_,, rp2040_rom_version, lua_pushinteger)
MLUA_FUNC_V1(mod_,, busy_wait_at_least_cycles, luaL_checkinteger)
MLUA_FUNC_R0(mod_,, get_core_num, lua_pushinteger)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(NUM_CORES, integer, NUM_CORES),
    MLUA_SYM_V(NUM_DMA_CHANNELS, integer, NUM_DMA_CHANNELS),
    MLUA_SYM_V(NUM_DMA_TIMERS, integer, NUM_DMA_TIMERS),
    MLUA_SYM_V(NUM_DMA_IRQS, integer, NUM_DMA_IRQS),
    MLUA_SYM_V(NUM_IRQS, integer, NUM_IRQS),
    MLUA_SYM_V(NUM_USER_IRQS, integer, NUM_USER_IRQS),
    MLUA_SYM_V(NUM_PIOS, integer, NUM_PIOS),
    MLUA_SYM_V(NUM_PIO_STATE_MACHINES, integer, NUM_PIO_STATE_MACHINES),
    MLUA_SYM_V(NUM_PIO_IRQS, integer, NUM_PIO_IRQS),
    MLUA_SYM_V(NUM_PWM_SLICES, integer, NUM_PWM_SLICES),
    MLUA_SYM_V(NUM_PWM_IRQS, integer, NUM_PWM_IRQS),
    MLUA_SYM_V(NUM_SPIN_LOCKS, integer, NUM_SPIN_LOCKS),
    MLUA_SYM_V(NUM_UARTS, integer, NUM_UARTS),
    MLUA_SYM_V(NUM_I2CS, integer, NUM_I2CS),
    MLUA_SYM_V(NUM_SPIS, integer, NUM_SPIS),
    MLUA_SYM_V(NUM_GENERIC_TIMERS, integer, NUM_GENERIC_TIMERS),
    MLUA_SYM_V(NUM_ALARMS, integer, NUM_ALARMS),
    MLUA_SYM_V(ADC_BASE_PIN, integer, ADC_BASE_PIN),
    MLUA_SYM_V(NUM_ADC_CHANNELS, integer, NUM_ADC_CHANNELS),
    MLUA_SYM_V(NUM_RESETS, integer, NUM_RESETS),
    MLUA_SYM_V(NUM_BANK0_GPIOS, integer, NUM_BANK0_GPIOS),
    MLUA_SYM_V(NUM_QSPI_GPIOS, integer, NUM_QSPI_GPIOS),
    MLUA_SYM_V(PIO_INSTRUCTION_COUNT, integer, PIO_INSTRUCTION_COUNT),
    MLUA_SYM_V(USBCTRL_DPRAM_SIZE, integer, USBCTRL_DPRAM_SIZE),
    MLUA_SYM_V(XOSC_KHZ, integer, XOSC_KHZ),
    MLUA_SYM_V(SYS_CLK_KHZ, integer, SYS_CLK_KHZ),
    MLUA_SYM_V(USB_CLK_KHZ, integer, USB_CLK_KHZ),
    MLUA_SYM_V(FIRST_USER_IRQ, integer, FIRST_USER_IRQ),
    MLUA_SYM_V(VTABLE_FIRST_IRQ, integer, VTABLE_FIRST_IRQ),
#ifdef PICO_RP2040
    MLUA_SYM_V(RP2040, integer, PICO_RP2040),
#else
    MLUA_SYM_V(RP2040, boolean, false),
#endif

    // panic_unsupported: Not useful in Lua
    // panic: Not useful in Lua
    MLUA_SYM_F(rp2040_chip_version, mod_),
    MLUA_SYM_F(rp2040_rom_version, mod_),
    // tight_loop_contents: Not useful in Lua
    MLUA_SYM_F(busy_wait_at_least_cycles, mod_),
    MLUA_SYM_F(get_core_num, mod_),
};

MLUA_OPEN_MODULE(pico.platform) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
