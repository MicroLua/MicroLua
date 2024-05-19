// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "lwip/dns.h"

#include <stdbool.h>
#include <string.h>

#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/int64.h"
#include "mlua/lwip.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

#ifndef MLUA_MAX_DNS_REQUESTS
#ifdef DNS_MAX_REQUESTS
#define MLUA_MAX_DNS_REQUESTS DNS_MAX_REQUESTS
#else
#define MLUA_MAX_DNS_REQUESTS DNS_TABLE_SIZE
#endif
#endif

static int mod_getserver(lua_State* ls) {
    u8_t index = luaL_checkinteger(ls, 1);
    ip_addr_t* res = mlua_new_IPAddr(ls);
    mlua_lwip_lock();
    *res = *dns_getserver(index);
    mlua_lwip_unlock();
    return 1;
}

static int mod_setserver(lua_State* ls) {
    u8_t index = luaL_checkinteger(ls, 1);
    ip_addr_t const* server = NULL;
    if (!lua_isnoneornil(ls, 2)) server = mlua_check_IPAddr(ls, 2);
    mlua_lwip_lock();
    dns_setserver(index, server);
    mlua_lwip_unlock();
    return 0;
}

typedef enum ReqStatus {
    STATUS_WAITING,
    STATUS_FOUND,
    STATUS_NOT_FOUND,
} ReqStatus;

typedef struct GHBNState {
    MLuaEvent event;
    ip_addr_t addr;
    uint8_t status;
} GHBNState;

static GHBNState ghbn_state[MLUA_MAX_DNS_REQUESTS];

static void handle_dns_found(char const* name, ip_addr_t const* addr,
                             void* arg) {
    GHBNState* state = arg;
    if (mlua_event_disable_abandoned(&state->event)) return;
    if (addr != NULL) {
        memcpy(&state->addr, addr, sizeof(state->addr));
        state->status = STATUS_FOUND;
    } else {
        state->status = STATUS_NOT_FOUND;
    }
    mlua_event_set(&state->event);
}

static int push_addr(lua_State* ls, ip_addr_t* addr) {
    memcpy(mlua_new_IPAddr(ls), addr, sizeof(*addr));
    return 1;
}

static int gethostbyname_loop(lua_State* ls, bool timeout) {
    if (timeout) return mlua_lwip_push_err(ls, ERR_TIMEOUT);
    GHBNState* state = lua_touserdata(ls, -1);
    mlua_lwip_lock();
    uint8_t status = state->status;
    mlua_lwip_unlock();
    switch (status) {
    case STATUS_FOUND: return push_addr(ls, &state->addr);
    case STATUS_NOT_FOUND: return lua_pushboolean(ls, false), 1;
    default: return -1;
    }
}

static int gethostbyname_done(lua_State* ls) {
    GHBNState* state = lua_touserdata(ls, lua_upvalueindex(1));
    mlua_lwip_lock();
    if (state->status == STATUS_WAITING) {
        mlua_event_abandon(ls, &state->event);
    } else {
        mlua_event_disable(ls, &state->event);
    }
    mlua_lwip_unlock();
    return 0;
}

static int mod_gethostbyname(lua_State* ls) {
    char const* hostname = luaL_checkstring(ls, 1);
    u8_t addrtype = luaL_optinteger(ls, 2, LWIP_DNS_ADDRTYPE_DEFAULT);
    int dlidx = 0;
    if (!lua_isnoneornil(ls, 3)) {
        mlua_push_deadline(ls, mlua_check_int64(ls, 3));
        lua_replace(ls, 3);
        dlidx = 3;
    }

    // Find an available slot.
    GHBNState* state = ghbn_state;
    for (; state != ghbn_state + MLUA_SIZE(ghbn_state); ++state) {
        if (mlua_event_enable(ls, &state->event)) break;
    }
    if (state == ghbn_state + MLUA_SIZE(ghbn_state)) {
        return mlua_lwip_push_err(ls, ERR_MEM);
    }
    lua_pushlightuserdata(ls, state);
    lua_pushcclosure(ls, &gethostbyname_done, 1);
    lua_toclose(ls, -1);
    state->status = STATUS_WAITING;
    lua_pushlightuserdata(ls, state);

    // Initiate the lookup and wait for the response.
    mlua_lwip_lock();
    err_t err = dns_gethostbyname_addrtype(
        hostname, &state->addr, &handle_dns_found, &state->event, addrtype);
    mlua_lwip_unlock();
    if (err == ERR_OK) {
        state->status = STATUS_FOUND;
        return push_addr(ls, &state->addr);
    }
    if (err != ERR_INPROGRESS) {
        state->status = STATUS_NOT_FOUND;
        return mlua_lwip_push_err(ls, err);
    }
    return mlua_event_wait(ls, &state->event, 0, &gethostbyname_loop, dlidx);
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(ADDRTYPE_DEFAULT, integer, LWIP_DNS_ADDRTYPE_DEFAULT),
    MLUA_SYM_V(ADDRTYPE_IPV4, integer, LWIP_DNS_ADDRTYPE_IPV4),
    MLUA_SYM_V(ADDRTYPE_IPV6, integer, LWIP_DNS_ADDRTYPE_IPV6),
    MLUA_SYM_V(ADDRTYPE_IPV4_IPV6, integer, LWIP_DNS_ADDRTYPE_IPV4_IPV6),
    MLUA_SYM_V(ADDRTYPE_IPV6_IPV4, integer, LWIP_DNS_ADDRTYPE_IPV6_IPV4),
    MLUA_SYM_V(MAX_SERVERS, integer, DNS_MAX_SERVERS),

    MLUA_SYM_F(getserver, mod_),
    MLUA_SYM_F(setserver, mod_),
    MLUA_SYM_F(gethostbyname, mod_),
};

MLUA_OPEN_MODULE(pico.lwip.dns) {
    mlua_thread_require(ls);
    mlua_require(ls, "mlua.int64", false);
    mlua_require(ls, "pico.lwip", false);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
