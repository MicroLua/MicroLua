// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "lwip/udp.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/lwip.h"
#include "mlua/module.h"
#include "mlua/util.h"

static char const UDP_name[] = "pico.lwip.UDP";

static struct udp_pcb** new_UDP(lua_State* ls) {
    struct udp_pcb** pcb = lua_newuserdatauv(ls, sizeof(struct udp_pcb*), 0);
    *pcb = NULL;
    luaL_getmetatable(ls, UDP_name);
    lua_setmetatable(ls, -2);
    return pcb;
}

static inline struct udp_pcb* check_UDP(lua_State* ls, int arg) {
    return *((struct udp_pcb**)luaL_checkudata(ls, arg, UDP_name));
}

static int UDP_sendto(lua_State* ls) {
    struct udp_pcb* pcb = check_UDP(ls, 1);
    struct pbuf* pb = mlua_check_PBUF(ls, 2);
    ip_addr_t* ip = mlua_check_IPAddr(ls, 3);
    u16_t port = luaL_checkinteger(ls, 4);
    mlua_lwip_lock();
    err_t err = udp_sendto(pcb, pb, ip, port);
    mlua_lwip_unlock();
    return mlua_lwip_push_result(ls, err);
}

static int UDP___close(lua_State* ls) {
    struct udp_pcb** pcb = luaL_checkudata(ls, 1, UDP_name);
    if (*pcb != NULL) {
        mlua_lwip_lock();
        udp_remove(*pcb);
        mlua_lwip_unlock();
        *pcb = NULL;
    }
    return 0;
}

MLUA_SYMBOLS(UDP_syms) = {
    MLUA_SYM_F(sendto, UDP_),
};

#define UDP___gc UDP___close

MLUA_SYMBOLS_NOHASH(UDP_syms_nh) = {
    MLUA_SYM_F_NH(__close, UDP_),
    MLUA_SYM_F_NH(__gc, UDP_),
};

static int mod_new(lua_State* ls) {
    struct udp_pcb** pcb = new_UDP(ls);
    mlua_lwip_lock();
    *pcb = udp_new();
    mlua_lwip_unlock();
    return *pcb != NULL ? 1 : 0;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(new, mod_),
};

MLUA_OPEN_MODULE(pico.lwip.udp) {
    mlua_require(ls, "pico.lwip", false);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the PBUF class.
    mlua_new_class(ls, UDP_name, UDP_syms, UDP_syms_nh);
    lua_pop(ls, 1);
    return 1;
}
