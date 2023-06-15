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

static MLuaReg const module_regs[] = {
#define MLUA_SYM(n) {.name=#n, .push=mlua_reg_push_integer, .integer=n}
    MLUA_SYM(clk_gpout0),
    MLUA_SYM(clk_gpout1),
    MLUA_SYM(clk_gpout2),
    MLUA_SYM(clk_gpout3),
    MLUA_SYM(clk_ref),
    MLUA_SYM(clk_sys),
    MLUA_SYM(clk_peri),
    MLUA_SYM(clk_usb),
    MLUA_SYM(clk_adc),
    MLUA_SYM(clk_rtc),
    MLUA_SYM(CLK_COUNT),
#undef MLUA_SYM
#define MLUA_SYM(n) MLUA_REG(integer, n, CLOCKS_ ## n)
    MLUA_SYM(FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY),
    MLUA_SYM(FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY),
    MLUA_SYM(FC0_SRC_VALUE_ROSC_CLKSRC),
    MLUA_SYM(FC0_SRC_VALUE_CLK_SYS),
    MLUA_SYM(FC0_SRC_VALUE_CLK_PERI),
    MLUA_SYM(FC0_SRC_VALUE_CLK_USB),
    MLUA_SYM(FC0_SRC_VALUE_CLK_ADC),
    MLUA_SYM(FC0_SRC_VALUE_CLK_RTC),
#undef MLUA_SYM
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(configure),
    MLUA_SYM(stop),
    MLUA_SYM(get_hz),
    MLUA_SYM(frequency_count_khz),
    MLUA_SYM(set_reported_hz),
    MLUA_SYM(gpio_init_int_frac),
    MLUA_SYM(gpio_init),
    MLUA_SYM(configure_gpin),
#undef MLUA_SYM
    {NULL},
};

int luaopen_hardware_clocks(lua_State* ls) {
    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
