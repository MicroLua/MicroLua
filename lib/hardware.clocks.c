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

static MLuaSym const module_syms[] = {
    MLUA_SYM_V(clk_gpout0, integer, clk_gpout0),
    MLUA_SYM_V(clk_gpout1, integer, clk_gpout1),
    MLUA_SYM_V(clk_gpout2, integer, clk_gpout2),
    MLUA_SYM_V(clk_gpout3, integer, clk_gpout3),
    MLUA_SYM_V(clk_ref, integer, clk_ref),
    MLUA_SYM_V(clk_sys, integer, clk_sys),
    MLUA_SYM_V(clk_peri, integer, clk_peri),
    MLUA_SYM_V(clk_usb, integer, clk_usb),
    MLUA_SYM_V(clk_adc, integer, clk_adc),
    MLUA_SYM_V(clk_rtc, integer, clk_rtc),
    MLUA_SYM_V(CLK_COUNT, integer, CLK_COUNT),
    MLUA_SYM_V(FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY, integer, CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY),
    MLUA_SYM_V(FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY, integer, CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY),
    MLUA_SYM_V(FC0_SRC_VALUE_ROSC_CLKSRC, integer, CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC),
    MLUA_SYM_V(FC0_SRC_VALUE_CLK_SYS, integer, CLOCKS_FC0_SRC_VALUE_CLK_SYS),
    MLUA_SYM_V(FC0_SRC_VALUE_CLK_PERI, integer, CLOCKS_FC0_SRC_VALUE_CLK_PERI),
    MLUA_SYM_V(FC0_SRC_VALUE_CLK_USB, integer, CLOCKS_FC0_SRC_VALUE_CLK_USB),
    MLUA_SYM_V(FC0_SRC_VALUE_CLK_ADC, integer, CLOCKS_FC0_SRC_VALUE_CLK_ADC),
    MLUA_SYM_V(FC0_SRC_VALUE_CLK_RTC, integer, CLOCKS_FC0_SRC_VALUE_CLK_RTC),

    MLUA_SYM_F(configure, mod_),
    MLUA_SYM_F(stop, mod_),
    MLUA_SYM_F(get_hz, mod_),
    MLUA_SYM_F(frequency_count_khz, mod_),
    MLUA_SYM_F(set_reported_hz, mod_),
    MLUA_SYM_F(gpio_init_int_frac, mod_),
    MLUA_SYM_F(gpio_init, mod_),
    MLUA_SYM_F(configure_gpin, mod_),
};

int luaopen_hardware_clocks(lua_State* ls) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
