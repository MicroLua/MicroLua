#include "hardware/clocks.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

MLUA_FUNC_1_5(mod_, clock_, configure, lua_pushboolean, luaL_checkinteger,
              luaL_checkinteger, luaL_checkinteger, luaL_checkinteger,
              luaL_checkinteger)
MLUA_FUNC_0_1(mod_, clock_, stop, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, clock_, get_hz, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_1_1(mod_,, frequency_count_khz, lua_pushinteger,
              luaL_checkinteger)
MLUA_FUNC_0_2(mod_, clock_, set_reported_hz, luaL_checkinteger,
              luaL_checkinteger)
MLUA_FUNC_0_4(mod_, clock_, gpio_init_int_frac, luaL_checkinteger,
              luaL_checkinteger, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_0_3(mod_, clock_, gpio_init, luaL_checkinteger,
              luaL_checkinteger, luaL_checknumber)
MLUA_FUNC_1_4(mod_, clock_, configure_gpin, lua_pushboolean, luaL_checkinteger,
              luaL_checkinteger, luaL_checkinteger, luaL_checkinteger)

static mlua_reg const module_regs[] = {
#define X(n) {.name=#n, .push=mlua_reg_push_integer, .integer=n}
    X(clk_gpout0),
    X(clk_gpout1),
    X(clk_gpout2),
    X(clk_gpout3),
    X(clk_ref),
    X(clk_sys),
    X(clk_peri),
    X(clk_usb),
    X(clk_adc),
    X(clk_rtc),
    X(CLK_COUNT),
#undef X
#define X(n) MLUA_REG(integer, FC0_SRC_VALUE_ ## n, CLOCKS_FC0_SRC_VALUE_ ## n)
    X(PLL_SYS_CLKSRC_PRIMARY),
    X(PLL_USB_CLKSRC_PRIMARY),
    X(ROSC_CLKSRC),
    X(CLK_SYS),
    X(CLK_PERI),
    X(CLK_USB),
    X(CLK_ADC),
    X(CLK_RTC),
#undef X
#define X(n) MLUA_REG(function, n, mod_ ## n)
    X(configure),
    X(stop),
    X(get_hz),
    X(frequency_count_khz),
    X(set_reported_hz),
    X(gpio_init_int_frac),
    X(gpio_init),
    X(configure_gpin),
#undef X
    {NULL},
};

int luaopen_hardware_clocks(lua_State* ls) {
    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
