// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "cyw43.h"
#include "pico/cyw43_driver.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/cyw43.h"
#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

typedef struct ScanResult {
    int16_t rssi;
    uint16_t channel;
    uint8_t auth_mode;
    uint8_t bssid[6];
    uint8_t ssid_len;
    uint8_t ssid[32];
} ScanResult;

typedef struct ScanQueue {
    uint8_t head, len, cap, dropped;
    ScanResult items[0];
} ScanQueue;

typedef struct WiFiState {
    MLuaEvent scan_event;
    ScanQueue* scan_queue;
} WiFiState;

static WiFiState wifi_state;

static int mod_set_up(lua_State* ls) {
    int itf = luaL_checkinteger(ls, 1);
    bool up = mlua_to_cbool(ls, 2);
    uint32_t country = luaL_checkinteger(ls, 3);
    cyw43_wifi_set_up(&cyw43_state, itf, up, country);
    return 0;
}

static int mod_pm(lua_State* ls) {
    uint32_t pm = luaL_checkinteger(ls, 1);
    int err = cyw43_wifi_pm(&cyw43_state, pm);
    return mlua_cyw43_push_result(ls, err);
}

static int mod_get_pm(lua_State* ls) {
    uint32_t pm;
    int err = cyw43_wifi_get_pm(&cyw43_state, &pm);
    if (err != 0) return mlua_cyw43_push_err(ls, err);
    return lua_pushinteger(ls, pm), 1;
}

static int mod_pm_value(lua_State* ls) {
    uint8_t pm_mode = luaL_checkinteger(ls, 1);
    uint16_t pm2_sleep_ret_ms = luaL_checkinteger(ls, 2);
    uint8_t li_beacon_period = luaL_checkinteger(ls, 3);
    uint8_t li_dtim_period = luaL_checkinteger(ls, 4);
    uint8_t li_assoc = luaL_checkinteger(ls, 5);
    uint32_t value = cyw43_pm_value(pm_mode, pm2_sleep_ret_ms, li_beacon_period,
                                    li_dtim_period, li_assoc);
    return lua_pushinteger(ls, value), 1;
}

static void mod_DEFAULT_PM(lua_State* ls, MLuaSymVal const* value) {
    lua_pushinteger(ls, CYW43_DEFAULT_PM);
}

static void mod_NONE_PM(lua_State* ls, MLuaSymVal const* value) {
    lua_pushinteger(ls, CYW43_NONE_PM);
}

static void mod_AGGRESSIVE_PM(lua_State* ls, MLuaSymVal const* value) {
    lua_pushinteger(ls, CYW43_AGGRESSIVE_PM);
}

static void mod_PERFORMANCE_PM(lua_State* ls, MLuaSymVal const* value) {
    lua_pushinteger(ls, CYW43_PERFORMANCE_PM);
}

static int mod_link_status(lua_State* ls) {
    int itf = luaL_checkinteger(ls, 1);
    return lua_pushinteger(ls, cyw43_wifi_link_status(&cyw43_state, itf)), 1;
}

static int mod_get_mac(lua_State* ls) {
    int itf = luaL_checkinteger(ls, 1);
    uint8_t mac[6];
    int err = cyw43_wifi_get_mac(&cyw43_state, itf, mac);
    if (err != 0) return mlua_cyw43_push_err(ls, err);
    return lua_pushlstring(ls, (char const*)mac, sizeof(mac)), 1;
}

static int mod_update_multicast_filter(lua_State* ls) {
    size_t len;
    uint8_t const* addr = (uint8_t const*)luaL_checklstring(ls, 1, &len);
    bool add = mlua_to_cbool(ls, 2);
    luaL_argcheck(ls, len == 6, 1, "invalid MAC");
    // BUG(cyw43-driver): The function takes a non-const pointer. This is likely
    // a mistake, as it doesn't modify it.
    int err = cyw43_wifi_update_multicast_filter(&cyw43_state, (uint8_t*)addr,
                                                 add);
    return mlua_cyw43_push_result(ls, err);
}

static int handle_scan_result(void* ptr, cyw43_ev_scan_result_t const* result) {
    mlua_event_lock();
    ScanQueue* q = wifi_state.scan_queue;
    if (q != NULL) {
        if (q->len < q->cap) {
            uint i = (uint)q->head + q->len;
            if (i >= q->cap) i -= q->cap;
            ++q->len;
            ScanResult* qe = &q->items[i];
            qe->rssi = result->rssi;
            qe->channel = result->channel;
            qe->auth_mode = result->auth_mode;
            qe->ssid_len = result->ssid_len;
            memcpy(qe->bssid, result->bssid, sizeof(result->bssid));
            memcpy(qe->ssid, result->ssid, result->ssid_len);
            mlua_event_set_nolock(&wifi_state.scan_event);
        } else {
            ++q->dropped;
        }
    }
    mlua_event_unlock();
    return 0;
}

static int scan_next_loop(lua_State* ls, bool timeout) {
    ScanQueue* q = lua_touserdata(ls, 1);
    ScanResult result;
    lua_Integer dropped = 0;
    mlua_event_lock();
    bool avail = q->len > 0;
    if (avail) {
        result = q->items[q->head];
        if (++q->head == q->cap) q->head = 0;
        --q->len;
        dropped = q->dropped;
        q->dropped = 0;
    }
    mlua_event_unlock();
    if (!avail) {
        if (!cyw43_wifi_scan_active(&cyw43_state)) return 0;
        if (timeout) {
            lua_pop(ls, 1);
            mlua_push_deadline(ls, 200000);
        }
        return -1;
    }

    // Return a result.
    lua_createtable(ls, 0, 5);
    lua_pushinteger(ls, result.rssi);
    lua_setfield(ls, -2, "rssi");
    lua_pushinteger(ls, result.channel);
    lua_setfield(ls, -2, "channel");
    lua_pushinteger(ls, result.auth_mode);
    lua_setfield(ls, -2, "auth_mode");
    lua_pushlstring(ls, (char*)result.bssid, sizeof(result.bssid));
    lua_setfield(ls, -2, "bssid");
    lua_pushlstring(ls, (char*)result.ssid, result.ssid_len);
    lua_setfield(ls, -2, "ssid");
    lua_pushinteger(ls, dropped);
    return 2;
}

static int scan_next(lua_State* ls) {
    lua_settop(ls, 1);
    mlua_push_deadline(ls, 200000);
    return mlua_event_wait(ls, &wifi_state.scan_event, 0, &scan_next_loop, 2);
}

static int scan_done(lua_State* ls) {
    mlua_event_lock();
    cyw43_state.wifi_scan_state = 2;  // Stop the scan
    wifi_state.scan_queue = NULL;
    mlua_event_unlock();
    mlua_event_disable(ls, &wifi_state.scan_event);
    return 0;
}

static void parse_scan_options(lua_State* ls, int arg,
                               cyw43_wifi_scan_options_t* opts) {
    if (lua_isnoneornil(ls, arg)) return;
    luaL_argexpected(ls, lua_istable(ls, arg), arg, "table or nil");
    if (lua_getfield(ls, arg, "ssid") != LUA_TNIL) {
        size_t len;
        char const* s = lua_tolstring(ls, -1, &len);
        luaL_argcheck(ls, s != NULL, arg, "ssid: expected string");
        luaL_argcheck(ls, len <= sizeof(opts->ssid), arg, "ssid: too long");
        opts->ssid_len = len;
        memcpy(opts->ssid, s, len);
    }
    lua_pop(ls, 1);
    if (lua_getfield(ls, arg, "scan_type") != LUA_TNIL) {
        luaL_argcheck(ls, lua_isinteger(ls, -1), arg,
                      "scan_type: expected integer");
        opts->scan_type = lua_tointeger(ls, -1);
    }
    lua_pop(ls, 1);
}

static int mod_scan(lua_State* ls) {
    cyw43_wifi_scan_options_t opts = {};
    parse_scan_options(ls, 1, &opts);
    lua_Integer cap = luaL_optinteger(ls, 2, 0);
    if (cap <= 0) cap = 8;
    luaL_argcheck(
        ls, (lua_Unsigned)cap < (1u << MLUA_SIZEOF_FIELD(ScanQueue, cap) * 8),
        2, "too large");
    lua_settop(ls, 0);

    // Prepare the iterator.
    lua_pushcfunction(ls, &scan_next);
    ScanQueue* q = lua_newuserdatauv(
        ls, sizeof(ScanQueue) + cap * sizeof(ScanResult), 0);
    q->head = q->len = q->dropped = 0;
    q->cap = cap;
    lua_pushnil(ls);
    lua_pushcfunction(ls, &scan_done);

    // Start the scan.
    if (!mlua_event_enable(ls, &wifi_state.scan_event)) {
        return luaL_error(ls, "WiFi: scan already in progress");
    }
    wifi_state.scan_queue = q;
    int err = cyw43_wifi_scan(&cyw43_state, &opts, NULL, &handle_scan_result);
    if (err != 0) {
        mlua_event_disable(ls, &wifi_state.scan_event);
        lua_pushstring(ls, mlua_pico_error_str(err));
        return lua_error(ls);
    }
    return 4;
}

static int mod_scan_active(lua_State* ls) {
    return lua_pushboolean(ls, cyw43_wifi_scan_active(&cyw43_state)), 1;
}

static int mod_join(lua_State* ls) {
    size_t ssid_len;
    uint8_t const* ssid = (uint8_t const*)luaL_checklstring(ls, 1, &ssid_len);
    size_t key_len;
    uint8_t const* key = (uint8_t const*)luaL_optlstring(ls, 2, NULL, &key_len);
    uint32_t auth_type = luaL_optinteger(ls, 3, CYW43_AUTH_OPEN);
    size_t bssid_len;
    uint8_t const* bssid = (uint8_t const*)luaL_optlstring(ls, 4, NULL,
                                                           &bssid_len);
    uint32_t channel = luaL_optinteger(ls, 5, CYW43_CHANNEL_NONE);
    luaL_argcheck(ls, auth_type == CYW43_AUTH_OPEN || key != NULL, 3,
                  "auth_type requires a key");
    luaL_argcheck(ls, (bssid_len == 0 && bssid == NULL) || bssid_len == 6, 4,
                  "invalid bssid length");
    int err = cyw43_wifi_join(&cyw43_state, ssid_len, ssid, key_len, key,
                              auth_type, bssid, channel);
    return mlua_cyw43_push_result(ls, err);
}

static int mod_leave(lua_State* ls) {
    uint32_t itf = luaL_checkinteger(ls, 1);
    int err = cyw43_wifi_leave(&cyw43_state, itf);
    return mlua_cyw43_push_result(ls, err);
}

static int mod_get_rssi(lua_State* ls) {
    int32_t rssi;
    int err = cyw43_wifi_get_rssi(&cyw43_state, &rssi);
    if (err != 0) mlua_cyw43_push_err(ls, err);
    return lua_pushinteger(ls, rssi), 1;
}

static int mod_get_bssid(lua_State* ls) {
    uint8_t bssid[6];
    int err = cyw43_wifi_get_bssid(&cyw43_state, bssid);
    if (err != 0) mlua_cyw43_push_err(ls, err);
    return lua_pushlstring(ls, (char const*)bssid, sizeof(bssid)), 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(SCAN_ACTIVE, integer, 0),
    MLUA_SYM_V(SCAN_PASSIVE, integer, 1),
    MLUA_SYM_P(DEFAULT_PM, mod_),
    MLUA_SYM_P(NONE_PM, mod_),
    MLUA_SYM_P(AGGRESSIVE_PM, mod_),
    MLUA_SYM_P(PERFORMANCE_PM, mod_),

    MLUA_SYM_F(set_up, mod_),
    MLUA_SYM_F(pm, mod_),
    MLUA_SYM_F(get_pm, mod_),
    MLUA_SYM_F(pm_value, mod_),
    MLUA_SYM_F(link_status, mod_),
    MLUA_SYM_F(get_mac, mod_),
    MLUA_SYM_F(update_multicast_filter, mod_),
    MLUA_SYM_F(scan, mod_),
    MLUA_SYM_F(scan_active, mod_),
    MLUA_SYM_F(join, mod_),
    MLUA_SYM_F(leave, mod_),
    MLUA_SYM_F(get_rssi, mod_),
    MLUA_SYM_F(get_bssid, mod_),
};

MLUA_OPEN_MODULE(pico.cyw43.wifi) {
    mlua_thread_require(ls);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
