// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "cyw43.h"
#include "pico/cyw43_driver.h"

#include "lua.h"
#include "lauxlib.h"
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

static int handle_scan_result_event_2(lua_State* ls, int status,
                                      lua_KContext ctx);

static int handle_scan_result_event(lua_State* ls) {
    ScanQueue* q = lua_touserdata(ls, lua_upvalueindex(1));
    ScanResult result;
    for (;;) {
        mlua_event_lock();
        bool avail = q->len > 0;
        if (avail) {
            result = q->items[q->head];
            ++q->head;
            if (q->head >= q->cap) q->head -= q->cap;
            --q->len;
        }
        lua_Integer dropped = q->dropped;
        q->dropped = 0;
        mlua_event_unlock();
        if (!avail) {
            if (!cyw43_wifi_scan_active(&cyw43_state)) {
                lua_pushnil(ls);
                return lua_error(ls);
            }
            return mlua_push_timeout_time(ls, 200000), 1;
        }

        // Call the handler
        lua_pushvalue(ls, lua_upvalueindex(2));  // handler
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
        lua_callk(ls, 2, 0, 0, &handle_scan_result_event_2);
    }
}

static int handle_scan_result_event_2(lua_State* ls, int status,
                                      lua_KContext ctx) {
    return handle_scan_result_event(ls);
}

static int scan_result_handler_done(lua_State* ls) {
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
    lua_Integer cap = luaL_optinteger(ls, 3, 0);
    if (cap <= 0) cap = 8;
    luaL_argcheck(
        ls, (lua_Unsigned)cap < (1u << MLUA_SIZEOF_FIELD(ScanQueue, cap) * 8),
        3, "too large");

    // Create scan queue.
    ScanQueue* q = lua_newuserdatauv(
        ls, sizeof(ScanQueue) + cap * sizeof(ScanResult), 0);
    q->head = q->len = q->dropped = 0;
    q->cap = cap;

    // Start scan.
    if (!mlua_event_enable(ls, &wifi_state.scan_event)) {
        return luaL_error(ls, "WiFi: scan already in progress");
    }
    wifi_state.scan_queue = q;
    int err = cyw43_wifi_scan(&cyw43_state, &opts, NULL, &handle_scan_result);
    if (err != 0) {
        mlua_event_disable(ls, &wifi_state.scan_event);
        mlua_push_fail(ls, "failed to start scan");
        lua_pushinteger(ls, err);
        return 3;
    }

    // Start the event handler thread.
    lua_pushvalue(ls, -1);  // queue
    lua_pushvalue(ls, 2);  // handler
    lua_pushcclosure(ls, &handle_scan_result_event, 2);
    lua_pushvalue(ls, -2);  // queue (to keep it alive)
    lua_pushcclosure(ls, &scan_result_handler_done, 1);
    return mlua_event_handle(ls, &wifi_state.scan_event,
                             &mlua_cont_return_ctx, 1);
}

static int mod_scan_active(lua_State* ls) {
    return lua_pushboolean(ls, cyw43_wifi_scan_active(&cyw43_state)), 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(SCAN_ACTIVE, integer, 0),
    MLUA_SYM_V(SCAN_PASSIVE, integer, 1),

    MLUA_SYM_F(set_up, mod_),
    MLUA_SYM_F(scan, mod_),
    MLUA_SYM_F(scan_active, mod_),
};

MLUA_OPEN_MODULE(pico.cyw43.wifi) {
    mlua_thread_require(ls);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
