// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>

#include "cyw43.h"
#include "cyw43_country.h"
#include "cyw43_ll.h"
#include "pico/cyw43_driver.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"
#include "mlua/util.h"

int mlua_cyw43_push_err(lua_State* ls, int err) {
    luaL_pushfail(ls);
    lua_pushinteger(ls, err);
    return 2;
}

int mlua_cyw43_push_result(lua_State* ls, int err) {
    if (luai_likely(err == 0)) return lua_pushboolean(ls, true), 1;
    return mlua_cyw43_push_err(ls, err);
}

static int mod_init(lua_State* ls) {
    async_context_t* ctx = mlua_async_context();
    bool res = cyw43_driver_init(ctx);
    if (!res) cyw43_driver_deinit(ctx);
    return lua_pushboolean(ls, res), 1;
}

static int mod_deinit(lua_State* ls) {
    cyw43_driver_deinit(mlua_async_context());
    return 0;
}

static int mod_is_initialized(lua_State* ls) {
    return lua_pushboolean(ls, cyw43_is_initialized(&cyw43_state)), 1;
}

static int mod_ioctl(lua_State* ls) {
    uint32_t cmd = luaL_checkinteger(ls, 1);
    size_t len;
    char const* data = luaL_checklstring(ls, 2, &len);
    uint32_t itf = luaL_checkinteger(ls, 3);
    luaL_Buffer buf;
    uint8_t* bdata = (uint8_t*)luaL_buffinitsize(ls, &buf, len);
    luaL_addlstring(&buf, data, len);
    int err = cyw43_ioctl(&cyw43_state, cmd, len, bdata, itf);
    if (err != 0) {
        return mlua_cyw43_push_err(ls, err);
    }
    luaL_pushresult(&buf);
    return 1;
}

#if CYW43_LWIP

static int mod_tcpip_link_status(lua_State* ls) {
    int itf = luaL_checkinteger(ls, 1);
    return lua_pushinteger(ls, cyw43_tcpip_link_status(&cyw43_state, itf)), 1;
}

#endif  // CYW43_LWIP

static int mod_link_status_str(lua_State* ls) {
    char const* msg = "unknown";
    switch (luaL_checkinteger(ls, 1)) {
    case CYW43_LINK_DOWN: msg = "link down"; break;
    case CYW43_LINK_JOIN: msg = "joining"; break;
    case CYW43_LINK_NOIP: msg = "no IP address"; break;
    case CYW43_LINK_UP: msg = "link up"; break;
    case CYW43_LINK_FAIL: msg = "connection failure"; break;
    case CYW43_LINK_NONET: msg = "no SSID found"; break;
    case CYW43_LINK_BADAUTH: msg = "authentication failure"; break;
    }
    return lua_pushstring(ls, msg), 1;
}

#if CYW43_GPIO

static int check_gpio(lua_State* ls, int arg) {
    lua_Unsigned num = luaL_checkinteger(ls, arg);
    luaL_argcheck(ls, num < CYW43_WL_GPIO_COUNT, arg, "invalid CYW43 GPIO");
    return num;
}

static int mod_gpio_set(lua_State* ls) {
    int gpio = check_gpio(ls, 1);
    bool value = mlua_to_cbool(ls, 2);
    int err = cyw43_gpio_set(&cyw43_state, gpio, value);
    return mlua_cyw43_push_result(ls, err);
}

static int mod_gpio_get(lua_State* ls) {
    int gpio = check_gpio(ls, 1);
    bool value;
    int err = cyw43_gpio_get(&cyw43_state, gpio, &value);
    if (err != 0) return luaL_pushfail(ls), 1;
    return lua_pushboolean(ls, value), 1;
}

#endif  // CYW43_GPIO

MLUA_SYMBOLS(module_syms) = {
    // TODO: Move WiFi-only symbols to pico.cyw43.wifi
    // From cyw43.h
    MLUA_SYM_V(VERSION, integer, CYW43_VERSION),
    MLUA_SYM_V(TRACE_ASYNC_EV, integer, CYW43_TRACE_ASYNC_EV),
    MLUA_SYM_V(TRACE_ETH_TX, integer, CYW43_TRACE_ETH_TX),
    MLUA_SYM_V(TRACE_ETH_RX, integer, CYW43_TRACE_ETH_RX),
    MLUA_SYM_V(TRACE_ETH_FULL, integer, CYW43_TRACE_ETH_FULL),
    MLUA_SYM_V(TRACE_MAC, integer, CYW43_TRACE_MAC),
    MLUA_SYM_V(LINK_DOWN, integer, CYW43_LINK_DOWN),
    MLUA_SYM_V(LINK_JOIN, integer, CYW43_LINK_JOIN),
    MLUA_SYM_V(LINK_NOIP, integer, CYW43_LINK_NOIP),
    MLUA_SYM_V(LINK_UP, integer, CYW43_LINK_UP),
    MLUA_SYM_V(LINK_FAIL, integer, CYW43_LINK_FAIL),
    MLUA_SYM_V(LINK_NONET, integer, CYW43_LINK_NONET),
    MLUA_SYM_V(LINK_BADAUTH, integer, CYW43_LINK_BADAUTH),
    // From cyw43_ll.h
    MLUA_SYM_V(AUTH_OPEN, integer, CYW43_AUTH_OPEN),
    MLUA_SYM_V(AUTH_WPA_TKIP_PSK, integer, CYW43_AUTH_WPA_TKIP_PSK),
    MLUA_SYM_V(AUTH_WPA2_AES_PSK, integer, CYW43_AUTH_WPA2_AES_PSK),
    MLUA_SYM_V(AUTH_WPA2_MIXED_PSK, integer, CYW43_AUTH_WPA2_MIXED_PSK),
    MLUA_SYM_V(AUTH_WPA3_SAE_AES_PSK, integer, CYW43_AUTH_WPA3_SAE_AES_PSK),
    MLUA_SYM_V(AUTH_WPA3_WPA2_AES_PSK, integer, CYW43_AUTH_WPA3_WPA2_AES_PSK),
    MLUA_SYM_V(NO_POWERSAVE_MODE, integer, CYW43_NO_POWERSAVE_MODE),
    MLUA_SYM_V(PM1_POWERSAVE_MODE, integer, CYW43_PM1_POWERSAVE_MODE),
    MLUA_SYM_V(PM2_POWERSAVE_MODE, integer, CYW43_PM2_POWERSAVE_MODE),
    MLUA_SYM_V(CHANNEL_NONE, integer, CYW43_CHANNEL_NONE),
    MLUA_SYM_V(ITF_STA, integer, CYW43_ITF_STA),
    MLUA_SYM_V(ITF_AP, integer, CYW43_ITF_AP),
    // From cyw43_country.h
    // TODO: Move to separate, auto-generated module
    MLUA_SYM_V(COUNTRY_WORLDWIDE, integer, CYW43_COUNTRY_WORLDWIDE),
    MLUA_SYM_V(COUNTRY_AUSTRALIA, integer, CYW43_COUNTRY_AUSTRALIA),
    MLUA_SYM_V(COUNTRY_AUSTRIA, integer, CYW43_COUNTRY_AUSTRIA),
    MLUA_SYM_V(COUNTRY_BELGIUM, integer, CYW43_COUNTRY_BELGIUM),
    MLUA_SYM_V(COUNTRY_BRAZIL, integer, CYW43_COUNTRY_BRAZIL),
    MLUA_SYM_V(COUNTRY_CANADA, integer, CYW43_COUNTRY_CANADA),
    MLUA_SYM_V(COUNTRY_CHILE, integer, CYW43_COUNTRY_CHILE),
    MLUA_SYM_V(COUNTRY_CHINA, integer, CYW43_COUNTRY_CHINA),
    MLUA_SYM_V(COUNTRY_COLOMBIA, integer, CYW43_COUNTRY_COLOMBIA),
    MLUA_SYM_V(COUNTRY_CZECH_REPUBLIC, integer, CYW43_COUNTRY_CZECH_REPUBLIC),
    MLUA_SYM_V(COUNTRY_DENMARK, integer, CYW43_COUNTRY_DENMARK),
    MLUA_SYM_V(COUNTRY_ESTONIA, integer, CYW43_COUNTRY_ESTONIA),
    MLUA_SYM_V(COUNTRY_FINLAND, integer, CYW43_COUNTRY_FINLAND),
    MLUA_SYM_V(COUNTRY_FRANCE, integer, CYW43_COUNTRY_FRANCE),
    MLUA_SYM_V(COUNTRY_GERMANY, integer, CYW43_COUNTRY_GERMANY),
    MLUA_SYM_V(COUNTRY_GREECE, integer, CYW43_COUNTRY_GREECE),
    MLUA_SYM_V(COUNTRY_HONG_KONG, integer, CYW43_COUNTRY_HONG_KONG),
    MLUA_SYM_V(COUNTRY_HUNGARY, integer, CYW43_COUNTRY_HUNGARY),
    MLUA_SYM_V(COUNTRY_ICELAND, integer, CYW43_COUNTRY_ICELAND),
    MLUA_SYM_V(COUNTRY_INDIA, integer, CYW43_COUNTRY_INDIA),
    MLUA_SYM_V(COUNTRY_ISRAEL, integer, CYW43_COUNTRY_ISRAEL),
    MLUA_SYM_V(COUNTRY_ITALY, integer, CYW43_COUNTRY_ITALY),
    MLUA_SYM_V(COUNTRY_JAPAN, integer, CYW43_COUNTRY_JAPAN),
    MLUA_SYM_V(COUNTRY_KENYA, integer, CYW43_COUNTRY_KENYA),
    MLUA_SYM_V(COUNTRY_LATVIA, integer, CYW43_COUNTRY_LATVIA),
    MLUA_SYM_V(COUNTRY_LIECHTENSTEIN, integer, CYW43_COUNTRY_LIECHTENSTEIN),
    MLUA_SYM_V(COUNTRY_LITHUANIA, integer, CYW43_COUNTRY_LITHUANIA),
    MLUA_SYM_V(COUNTRY_LUXEMBOURG, integer, CYW43_COUNTRY_LUXEMBOURG),
    MLUA_SYM_V(COUNTRY_MALAYSIA, integer, CYW43_COUNTRY_MALAYSIA),
    MLUA_SYM_V(COUNTRY_MALTA, integer, CYW43_COUNTRY_MALTA),
    MLUA_SYM_V(COUNTRY_MEXICO, integer, CYW43_COUNTRY_MEXICO),
    MLUA_SYM_V(COUNTRY_NETHERLANDS, integer, CYW43_COUNTRY_NETHERLANDS),
    MLUA_SYM_V(COUNTRY_NEW_ZEALAND, integer, CYW43_COUNTRY_NEW_ZEALAND),
    MLUA_SYM_V(COUNTRY_NIGERIA, integer, CYW43_COUNTRY_NIGERIA),
    MLUA_SYM_V(COUNTRY_NORWAY, integer, CYW43_COUNTRY_NORWAY),
    MLUA_SYM_V(COUNTRY_PERU, integer, CYW43_COUNTRY_PERU),
    MLUA_SYM_V(COUNTRY_PHILIPPINES, integer, CYW43_COUNTRY_PHILIPPINES),
    MLUA_SYM_V(COUNTRY_POLAND, integer, CYW43_COUNTRY_POLAND),
    MLUA_SYM_V(COUNTRY_PORTUGAL, integer, CYW43_COUNTRY_PORTUGAL),
    MLUA_SYM_V(COUNTRY_SINGAPORE, integer, CYW43_COUNTRY_SINGAPORE),
    MLUA_SYM_V(COUNTRY_SLOVAKIA, integer, CYW43_COUNTRY_SLOVAKIA),
    MLUA_SYM_V(COUNTRY_SLOVENIA, integer, CYW43_COUNTRY_SLOVENIA),
    MLUA_SYM_V(COUNTRY_SOUTH_AFRICA, integer, CYW43_COUNTRY_SOUTH_AFRICA),
    MLUA_SYM_V(COUNTRY_SOUTH_KOREA, integer, CYW43_COUNTRY_SOUTH_KOREA),
    MLUA_SYM_V(COUNTRY_SPAIN, integer, CYW43_COUNTRY_SPAIN),
    MLUA_SYM_V(COUNTRY_SWEDEN, integer, CYW43_COUNTRY_SWEDEN),
    MLUA_SYM_V(COUNTRY_SWITZERLAND, integer, CYW43_COUNTRY_SWITZERLAND),
    MLUA_SYM_V(COUNTRY_TAIWAN, integer, CYW43_COUNTRY_TAIWAN),
    MLUA_SYM_V(COUNTRY_THAILAND, integer, CYW43_COUNTRY_THAILAND),
    MLUA_SYM_V(COUNTRY_TURKEY, integer, CYW43_COUNTRY_TURKEY),
    MLUA_SYM_V(COUNTRY_UK, integer, CYW43_COUNTRY_UK),
    MLUA_SYM_V(COUNTRY_USA, integer, CYW43_COUNTRY_USA),

    MLUA_SYM_F(init, mod_),
    MLUA_SYM_F(deinit, mod_),
    MLUA_SYM_F(is_initialized, mod_),
    MLUA_SYM_F(ioctl, mod_),
#if CYW43_LWIP
    MLUA_SYM_F(tcpip_link_status, mod_),
#else
    MLUA_SYM_V(tcpip_link_status, boolean, false),
#endif
    MLUA_SYM_F(link_status_str, mod_),
#if CYW43_GPIO
    MLUA_SYM_F(gpio_set, mod_),
    MLUA_SYM_F(gpio_get, mod_),
#else
    MLUA_SYM_V(gpio_set, boolean, false),
    MLUA_SYM_V(gpio_get, boolean, false),
#endif
};

MLUA_OPEN_MODULE(pico.cyw43) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
