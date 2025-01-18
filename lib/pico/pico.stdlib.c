// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "pico/stdlib.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"
#include "mlua/util.h"

MLUA_FUNC_V0(mod_,, setup_default_uart)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(DEFAULT_LED_PIN_INVERTED, boolean, PICO_DEFAULT_LED_PIN_INVERTED),
#ifdef PICO_DEFAULT_WS2812_PIN
    MLUA_SYM_V(DEFAULT_WS2812_PIN, integer, PICO_DEFAULT_WS2812_PIN),
#else
    MLUA_SYM_V(DEFAULT_WS2812_PIN, boolean, false),
#endif
#ifdef PICO_DEFAULT_WS2812_POWER_PIN
    MLUA_SYM_V(DEFAULT_WS2812_POWER_PIN, integer, PICO_DEFAULT_WS2812_POWER_PIN),
#else
    MLUA_SYM_V(DEFAULT_WS2812_POWER_PIN, boolean, false),
#endif

    MLUA_SYM_F(setup_default_uart, mod_),
};

MLUA_OPEN_MODULE(pico.stdlib) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
