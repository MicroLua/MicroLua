// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "lwip/udp.h"

#include <stdbool.h>

#include "lua.h"
#include "lauxlib.h"
#include "mlua/lwip.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

typedef struct Packet {
    struct pbuf* p;
    ip_addr_t addr;
    u16_t port;
} Packet;

typedef struct UDP {
    struct udp_pcb* pcb;
    MLuaEvent event;
    uint8_t head, len, cap, dropped;
    Packet packets[0];
} UDP;

static void handle_recv(void* arg, struct udp_pcb* pcb, struct pbuf* p,
                        ip_addr_t const* addr, u16_t port) {
    UDP* udp = arg;
    if (udp->len < udp->cap) {
        uint i = (uint)udp->head + udp->len;
        if (i >= udp->cap) i -= udp->cap;
        ++udp->len;
        Packet* pkt = &udp->packets[i];
        pkt->p = p;
        pkt->addr = *addr;
        pkt->port = port;
        mlua_event_set(&udp->event);
    } else {
        pbuf_free(p);
        ++udp->dropped;
    }
}

static char const UDP_name[] = "pico.lwip.UDP";

static inline UDP* check_UDP(lua_State* ls, int arg) {
    return luaL_checkudata(ls, arg, UDP_name);
}

static inline UDP* to_UDP(lua_State* ls, int arg) {
    return lua_touserdata(ls, arg);
}

static int UDP_free(lua_State* ls) {
    UDP* udp = luaL_checkudata(ls, 1, UDP_name);
    if (udp->pcb == NULL) return 0;
    mlua_lwip_lock();
    udp_remove(udp->pcb);
    mlua_lwip_unlock();
    udp->pcb = NULL;
    if (lua_rawlen(ls, 1) == sizeof(struct pbuf*)) return 0;
    mlua_event_disable(ls, &udp->event);
    for (; udp->len > 0; --udp->len) {
        Packet* pkt = &udp->packets[udp->head];
        mlua_lwip_lock();
        pbuf_free(pkt->p);
        mlua_lwip_unlock();
        if (++udp->head >= udp->cap) udp->head = 0;
    }
    return 0;
}

static int UDP_sendto(lua_State* ls) {
    UDP* udp = check_UDP(ls, 1);
    luaL_argcheck(ls, udp->pcb != NULL, 1, "closed");
    struct pbuf* pb = mlua_check_PBUF(ls, 2);
    ip_addr_t* ip = mlua_check_IPAddr(ls, 3);
    u16_t port = luaL_checkinteger(ls, 4);
    mlua_lwip_lock();
    err_t err = udp_sendto(udp->pcb, pb, ip, port);
    mlua_lwip_unlock();
    return mlua_lwip_push_result(ls, err);
}

static int recv_loop(lua_State* ls, bool timeout) {
    if (timeout) return 0;
    UDP* udp = to_UDP(ls, 1);
    mlua_lwip_lock();
    bool empty = udp->len == 0;
    mlua_lwip_unlock();
    if (empty) return -1;
    Packet* pkt = &udp->packets[udp->head];
    *mlua_new_PBUF(ls) = pkt->p;
    *mlua_new_IPAddr(ls) = pkt->addr;
    lua_pushinteger(ls, pkt->port);
    mlua_lwip_lock();
    if (++udp->head == udp->cap) udp->head = 0;
    --udp->len;
    mlua_lwip_unlock();
    return 3;
}

static int UDP_recv(lua_State* ls) {
    UDP* udp = luaL_checkudata(ls, 1, UDP_name);
    luaL_argcheck(ls, udp->pcb != NULL, 1, "closed");
    return mlua_event_wait(ls, &udp->event, 0, &recv_loop, 2);
}

MLUA_SYMBOLS(UDP_syms) = {
    MLUA_SYM_F(free, UDP_),
    // MLUA_SYM_F(bind, UDP_),
    // MLUA_SYM_F(connect, UDP_),
    // MLUA_SYM_F(disconnect, UDP_),
    // MLUA_SYM_F(send, UDP_),
    MLUA_SYM_F(sendto, UDP_),
    MLUA_SYM_F(recv, UDP_),
};

#define UDP___close UDP_free
#define UDP___gc UDP_free

MLUA_SYMBOLS_NOHASH(UDP_syms_nh) = {
    MLUA_SYM_F_NH(__close, UDP_),
    MLUA_SYM_F_NH(__gc, UDP_),
};

static int mod_new(lua_State* ls) {
    u8_t type = luaL_optinteger(ls, 1, IPADDR_TYPE_ANY);
    lua_Integer cap = luaL_optinteger(ls, 2, 0);
    if (cap < 0) cap = 0;
    luaL_argcheck(
        ls, (lua_Unsigned)cap < (1u << MLUA_SIZEOF_FIELD(UDP, cap) * 8),
        2, "too large");
    size_t size = cap > 0 ? sizeof(UDP) + cap * sizeof(Packet)
                  : sizeof(struct pbuf*);
    UDP* udp = lua_newuserdatauv(ls, size, 0);
    udp->pcb = NULL;
    luaL_getmetatable(ls, UDP_name);
    lua_setmetatable(ls, -2);
    mlua_lwip_lock();
    udp->pcb = udp_new_ip_type(type);
    mlua_lwip_unlock();
    if (udp->pcb == NULL) return 0;
    if (cap == 0) return 1;
    udp->cap = cap;
    udp->head = udp->len = udp->dropped = 0;
    mlua_event_init(&udp->event);
    mlua_event_enable(ls, &udp->event);
    mlua_lwip_lock();
    udp_recv(udp->pcb, &handle_recv, udp);
    mlua_lwip_unlock();
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(new, mod_),
};

MLUA_OPEN_MODULE(pico.lwip.udp) {
    mlua_thread_require(ls);
    mlua_require(ls, "pico.lwip", false);
    mlua_require(ls, "pico.lwip.pbuf", false);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the PBUF class.
    mlua_new_class(ls, UDP_name, UDP_syms, UDP_syms_nh);
    lua_pop(ls, 1);
    return 1;
}
