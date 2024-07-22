// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>

#include "lwip/netif.h"
#include "lwip/udp.h"

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
    MLuaEvent recv_event;
    uint8_t head, len, cap, dropped;
    Packet recv_packets[0];
} UDP;

static void handle_recv(void* arg, struct udp_pcb* pcb, struct pbuf* p,
                        ip_addr_t const* addr, u16_t port) {
    UDP* udp = arg;
    if (udp->len < udp->cap) {
        uint i = (uint)udp->head + udp->len;
        if (i >= udp->cap) i -= udp->cap;
        ++udp->len;
        Packet* pkt = &udp->recv_packets[i];
        pkt->p = p;
        pkt->addr = *addr;
        pkt->port = port;
        mlua_event_set(&udp->recv_event);
    } else {
        pbuf_free(p);
        ++udp->dropped;
    }
}

static char const UDP_name[] = "lwip.UDP";

static inline UDP* check_UDP(lua_State* ls, int arg) {
    return luaL_checkudata(ls, arg, UDP_name);
}

static inline UDP* to_UDP(lua_State* ls, int arg) {
    return lua_touserdata(ls, arg);
}

static int UDP_close(lua_State* ls) {
    UDP* udp = check_UDP(ls, 1);
    if (udp->pcb == NULL) return 0;
    mlua_lwip_lock();
    udp_remove(udp->pcb);
    mlua_lwip_unlock();
    udp->pcb = NULL;
    if (lua_rawlen(ls, 1) == sizeof(struct udp_pcb*)) return 0;
    mlua_event_disable(ls, &udp->recv_event);
    for (; udp->len > 0; --udp->len) {
        Packet* pkt = &udp->recv_packets[udp->head];
        mlua_lwip_lock();
        pbuf_free(pkt->p);
        mlua_lwip_unlock();
        if (++udp->head >= udp->cap) udp->head = 0;
    }
    return 0;
}

static int UDP_bind(lua_State* ls) {
    UDP* udp = check_UDP(ls, 1);
    if (udp->pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    ip_addr_t const* addr = luaL_opt(ls, mlua_check_IPAddr, 2, IP_ANY_TYPE);
    u16_t port = luaL_checkinteger(ls, 3);
    mlua_lwip_lock();
    err_t err = udp_bind(udp->pcb, addr, port);
    mlua_lwip_unlock();
    return mlua_lwip_push_result(ls, err);
}

static int UDP_bind_netif(lua_State* ls) {
    UDP* udp = check_UDP(ls, 1);
    if (udp->pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    struct netif* netif = luaL_opt(ls, mlua_check_NETIF, 2, NULL);
    mlua_lwip_lock();
    udp_bind_netif(udp->pcb, netif);
    mlua_lwip_unlock();
    return lua_pushboolean(ls, true), 1;
}

static int UDP_connect(lua_State* ls) {
    UDP* udp = check_UDP(ls, 1);
    if (udp->pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    ip_addr_t const* addr = luaL_opt(ls, mlua_check_IPAddr, 2, IP_ANY_TYPE);
    u16_t port = luaL_checkinteger(ls, 3);
    mlua_lwip_lock();
    err_t err = udp_connect(udp->pcb, addr, port);
    mlua_lwip_unlock();
    return mlua_lwip_push_result(ls, err);
}

static int UDP_disconnect(lua_State* ls) {
    UDP* udp = check_UDP(ls, 1);
    if (udp->pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    mlua_lwip_lock();
    udp_disconnect(udp->pcb);
    mlua_lwip_unlock();
    return 0;
}

static int UDP_send(lua_State* ls) {
    UDP* udp = check_UDP(ls, 1);
    if (udp->pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    struct pbuf* pb = mlua_check_PBUF(ls, 2);
    mlua_lwip_lock();
    err_t err = udp_send(udp->pcb, pb);
    mlua_lwip_unlock();
    return mlua_lwip_push_result(ls, err);
}

static int UDP_sendto(lua_State* ls) {
    UDP* udp = check_UDP(ls, 1);
    if (udp->pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    struct pbuf* pb = mlua_check_PBUF(ls, 2);
    ip_addr_t* addr = mlua_check_IPAddr(ls, 3);
    u16_t port = luaL_checkinteger(ls, 4);
    struct netif* netif = luaL_opt(ls, mlua_check_NETIF, 5, NULL);
    ip_addr_t* src = luaL_opt(ls, mlua_check_IPAddr, 6, NULL);
    luaL_argcheck(ls, netif != NULL || src == NULL, 5,
                  "required if src is specified");
    mlua_lwip_lock();
    err_t err;
    if (netif == NULL) {
        err = udp_sendto(udp->pcb, pb, addr, port);
    } else if (src == NULL) {
        err = udp_sendto_if(udp->pcb, pb, addr, port, netif);
    } else {
        err = udp_sendto_if_src(udp->pcb, pb, addr, port, netif, src);
    }
    mlua_lwip_unlock();
    return mlua_lwip_push_result(ls, err);
}

static int recv_loop(lua_State* ls, bool timeout) {
    UDP* udp = to_UDP(ls, 1);
    mlua_lwip_lock();
    bool empty = udp->len == 0;
    mlua_lwip_unlock();
    if (empty) {
        if (!timeout) return -1;
        return mlua_lwip_push_err(ls, ERR_TIMEOUT);
    }
    bool from = lua_toboolean(ls, 3);
    Packet* pkt = &udp->recv_packets[udp->head];
    *mlua_new_PBUF(ls) = pkt->p;
    if (from) {
        *mlua_new_IPAddr(ls) = pkt->addr;
        lua_pushinteger(ls, pkt->port);
    }
    mlua_lwip_lock();
    if (++udp->head == udp->cap) udp->head = 0;
    --udp->len;
    mlua_lwip_unlock();
    return from ? 3 : 1;
}

static int UDP_recv(lua_State* ls) {
    UDP* udp = check_UDP(ls, 1);
    if (udp->pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    lua_settop(ls, 2);  // Ensure deadline is set
    return mlua_event_wait(ls, &udp->recv_event, 0, &recv_loop, 2);
}

static int UDP_recvfrom(lua_State* ls) {
    UDP* udp = check_UDP(ls, 1);
    if (udp->pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    lua_settop(ls, 2);  // Ensure deadline is set
    lua_pushboolean(ls, true);
    return mlua_event_wait(ls, &udp->recv_event, 0, &recv_loop, 2);
}

static int UDP_local_ip(lua_State* ls) {
    struct udp_pcb* pcb = check_UDP(ls, 1)->pcb;
    if (pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    ip_addr_t* addr = mlua_new_IPAddr(ls);
    mlua_lwip_lock();
    *addr = pcb->local_ip;
    mlua_lwip_unlock();
    return 1;
}

static int UDP_remote_ip(lua_State* ls) {
    struct udp_pcb* pcb = check_UDP(ls, 1)->pcb;
    if (pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    ip_addr_t* addr = mlua_new_IPAddr(ls);
    mlua_lwip_lock();
    *addr = pcb->remote_ip;
    mlua_lwip_unlock();
    return 1;
}

static int UDP_local_port(lua_State* ls) {
    struct udp_pcb* pcb = check_UDP(ls, 1)->pcb;
    if (pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    mlua_lwip_lock();
    u16_t port = pcb->local_port;
    mlua_lwip_unlock();
    return lua_pushinteger(ls, port), 1;
}

static int UDP_remote_port(lua_State* ls) {
    struct udp_pcb* pcb = check_UDP(ls, 1)->pcb;
    if (pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    mlua_lwip_lock();
    u16_t port = pcb->remote_port;
    mlua_lwip_unlock();
    return lua_pushinteger(ls, port), 1;
}

static int UDP_options(lua_State* ls) {
    struct udp_pcb* pcb = check_UDP(ls, 1)->pcb;
    if (pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    u8_t set = luaL_optinteger(ls, 2, 0);
    u8_t clear = luaL_optinteger(ls, 3, 0);
    mlua_lwip_lock();
    u8_t old = pcb->so_options;
    pcb->so_options = (pcb->so_options | set) & ~clear;
    mlua_lwip_unlock();
    return lua_pushinteger(ls, old), 1;
}

static int UDP_tos(lua_State* ls) {
    struct udp_pcb* pcb = check_UDP(ls, 1)->pcb;
    if (pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    lua_Integer value = luaL_optinteger(ls, 2, -1);
    mlua_lwip_lock();
    u8_t old = pcb->tos;
    if (value != -1) pcb->tos = value;
    mlua_lwip_unlock();
    return lua_pushinteger(ls, old), 1;
}

static int UDP_ttl(lua_State* ls) {
    struct udp_pcb* pcb = check_UDP(ls, 1)->pcb;
    if (pcb == NULL) return mlua_lwip_push_err(ls, ERR_CLSD);
    lua_Integer value = luaL_optinteger(ls, 2, -1);
    mlua_lwip_lock();
    u8_t old = pcb->ttl;
    if (value != -1) pcb->ttl = value;
    mlua_lwip_unlock();
    return lua_pushinteger(ls, old), 1;
}

MLUA_SYMBOLS(UDP_syms) = {
    MLUA_SYM_F(close, UDP_),
    MLUA_SYM_F(bind, UDP_),
    MLUA_SYM_F(bind_netif, UDP_),
    MLUA_SYM_F(connect, UDP_),
    MLUA_SYM_F(disconnect, UDP_),
    MLUA_SYM_F(send, UDP_),
    MLUA_SYM_F(sendto, UDP_),
    MLUA_SYM_F(recv, UDP_),
    MLUA_SYM_F(recvfrom, UDP_),
    MLUA_SYM_F(local_ip, UDP_),
    MLUA_SYM_F(remote_ip, UDP_),
    MLUA_SYM_F(local_port, UDP_),
    MLUA_SYM_F(remote_port, UDP_),
    MLUA_SYM_F(options, UDP_),
    MLUA_SYM_F(tos, UDP_),
    MLUA_SYM_F(ttl, UDP_),
};

#define UDP___close UDP_close
#define UDP___gc UDP_close

MLUA_SYMBOLS_NOHASH(UDP_syms_nh) = {
    MLUA_SYM_F_NH(__close, UDP_),
    MLUA_SYM_F_NH(__gc, UDP_),
};

static int mod_new(lua_State* ls) {
    u8_t type = luaL_optinteger(ls, 1, IPADDR_TYPE_ANY);
    lua_Integer cap = luaL_optinteger(ls, 2, 4);
    if (cap < 0) cap = 0;
    luaL_argcheck(
        ls, (lua_Unsigned)cap < (1u << MLUA_SIZEOF_FIELD(UDP, cap) * 8),
        2, "too large");
    size_t size = cap > 0 ? sizeof(UDP) + cap * sizeof(Packet)
                  : sizeof(struct udp_pcb*);
    UDP* udp = lua_newuserdatauv(ls, size, 0);
    udp->pcb = NULL;
    luaL_getmetatable(ls, UDP_name);
    lua_setmetatable(ls, -2);
    mlua_lwip_lock();
    udp->pcb = udp_new_ip_type(type);
    mlua_lwip_unlock();
    if (udp->pcb == NULL) return mlua_lwip_push_err(ls, ERR_MEM);
    if (cap == 0) return 1;
    udp->cap = cap;
    udp->head = udp->len = udp->dropped = 0;
    mlua_event_init(&udp->recv_event);
    mlua_event_enable(ls, &udp->recv_event);
    mlua_lwip_lock();
    udp_recv(udp->pcb, &handle_recv, udp);
    mlua_lwip_unlock();
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(new, mod_),
};

MLUA_OPEN_MODULE(lwip.udp) {
    mlua_thread_require(ls);
    mlua_require(ls, "lwip", false);
    mlua_require(ls, "lwip.pbuf", false);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the UDP class.
    mlua_new_class(ls, UDP_name, UDP_syms, UDP_syms_nh);
    lua_pop(ls, 1);
    return 1;
}
