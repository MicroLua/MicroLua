// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <string.h>

#include "lwip/ip_addr.h"
#include "lwip/netif.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/lwip.h"
#include "mlua/module.h"
#include "mlua/util.h"

char const mlua_NETIF_name[] = "lwip.NETIF";

struct netif** new_NETIF(lua_State* ls) {
    struct netif** netif = lua_newuserdatauv(ls, sizeof(struct netif*), 0);
    *netif = NULL;
    luaL_getmetatable(ls, mlua_NETIF_name);
    lua_setmetatable(ls, -2);
    return netif;
}

static int NETIF_index(lua_State* ls) {
    struct netif* netif = mlua_check_NETIF(ls, 1);
    mlua_lwip_lock();
    u8_t index = netif_get_index(netif);
    mlua_lwip_unlock();
    if (index == NETIF_NO_INDEX) return mlua_lwip_push_err(ls, ERR_ARG);
    return lua_pushinteger(ls, index), 1;
}

static int NETIF_name(lua_State* ls) {
    struct netif* netif = mlua_check_NETIF(ls, 1);
    char name[NETIF_NAMESIZE];
    mlua_lwip_lock();
    char const* res = netif_index_to_name(netif_get_index(netif), name);
    mlua_lwip_unlock();
    if (res == NULL) return mlua_lwip_push_err(ls, ERR_ARG);
    return lua_pushstring(ls, res), 1;
}

static int NETIF_flags(lua_State* ls) {
    struct netif* netif = mlua_check_NETIF(ls, 1);
    mlua_lwip_lock();
    u8_t flags = netif->flags;
    mlua_lwip_unlock();
    return lua_pushinteger(ls, flags), 1;
}

static int NETIF_hwaddr(lua_State* ls) {
    struct netif* netif = mlua_check_NETIF(ls, 1);
    size_t addr_len = 0;
    char const* addr = luaL_optlstring(ls, 2, NULL, &addr_len);
    luaL_argcheck(ls, addr_len <= NETIF_MAX_HWADDR_LEN, 2, "address too long");
    u8_t hwaddr[NETIF_MAX_HWADDR_LEN];
    mlua_lwip_lock();
    u8_t len = netif->hwaddr_len;
    memcpy(hwaddr, netif->hwaddr, len);
    if (addr != NULL) {
        memcpy(netif->hwaddr, addr, addr_len);
        netif->hwaddr_len = addr_len;
    }
    mlua_lwip_unlock();
    return lua_pushlstring(ls, (char const*)hwaddr, len), 1;
}

static int NETIF_ip4(lua_State* ls) {
#if LWIP_IPV4
    struct netif* netif = mlua_check_NETIF(ls, 1);
    mlua_lwip_lock();
    ip_addr_t ip = *netif_ip_addr4(netif);
    ip_addr_t mask = *netif_ip_netmask4(netif);
    ip_addr_t gw = *netif_ip_gw4(netif);
    mlua_lwip_unlock();
    *mlua_new_IPAddr(ls) = ip;
    *mlua_new_IPAddr(ls) = mask;
    *mlua_new_IPAddr(ls) = gw;
    return 3;
#else
    return mlua_lwip_push_err(ls, ERR_ARG);
#endif
}

#if LWIP_IPV4

static ip4_addr_t const* opt_ip4(lua_State* ls, int arg,
                                 ip4_addr_t const* def) {
    if (lua_isnoneornil(ls, arg)) return def;
    return ip_2_ip4(mlua_check_IPAddr4(ls, arg));
}

#endif  // LWIP_IPV4

static int NETIF_set_ip4(lua_State* ls) {
#if LWIP_IPV4
    struct netif* netif = mlua_check_NETIF(ls, 1);
    ip4_addr_t const* ip = opt_ip4(ls, 2, netif_ip4_addr(netif));
    ip4_addr_t const* mask = opt_ip4(ls, 3, netif_ip4_netmask(netif));
    ip4_addr_t const* gw = opt_ip4(ls, 4, netif_ip4_gw(netif));
    mlua_lwip_lock();
    netif_set_addr(netif, ip, mask, gw);
    mlua_lwip_unlock();
    return lua_pushboolean(ls, true), 1;
#else
    return mlua_lwip_push_err(ls, ERR_ARG);
#endif
}

#if LWIP_IPV6

static void get_ip6_item(lua_State* ls, struct netif* netif, u8_t idx) {
    mlua_lwip_lock();
    ip_addr_t ip = *netif_ip_addr6(netif, idx);
    u8_t state = netif_ip6_addr_state(netif, idx);
#if LWIP_IPV6_ADDRESS_LIFETIMES
    u32_t valid = netif_ip6_addr_valid_life(netif, idx);
    u32_t pref = netif_ip6_addr_pref_life(netif, idx);
#endif
    mlua_lwip_unlock();
    *mlua_new_IPAddr(ls) = ip;
    lua_pushinteger(ls, state);
#if LWIP_IPV6_ADDRESS_LIFETIMES
    lua_pushinteger(ls, valid);
    lua_pushinteger(ls, pref);
#endif
}

#endif  // LWIP_IPV6

static int NETIF_ip6_next(lua_State* ls) {
#if LWIP_IPV6
    struct netif* netif = mlua_check_NETIF(ls, 1);
    u8_t idx = lua_isnoneornil(ls, 2) ? 0 : luaL_checkinteger(ls, 2) + 1;
    if (idx >= LWIP_IPV6_NUM_ADDRESSES) return 0;
    lua_pushinteger(ls, idx);
    get_ip6_item(ls, netif, idx);
    return LWIP_IPV6_ADDRESS_LIFETIMES ? 5 : 3;
#else
    return 0;
#endif
}

static int NETIF_ip6(lua_State* ls) {
    struct netif* netif = mlua_check_NETIF(ls, 1);
    if (lua_isnoneornil(ls, 2)) {
        lua_pushcfunction(ls, &NETIF_ip6_next);
        lua_rotate(ls, 1, -1);
        return 2;
    }
#if LWIP_IPV6
    lua_Unsigned idx = luaL_checkinteger(ls, 2);
    if (idx >= LWIP_IPV6_NUM_ADDRESSES) return mlua_lwip_push_err(ls, ERR_ARG);
    get_ip6_item(ls, netif, idx);
    return LWIP_IPV6_ADDRESS_LIFETIMES ? 4 : 2;
#else
    return mlua_lwip_push_err(ls, ERR_ARG);
#endif
}

static int NETIF_set_ip6(lua_State* ls) {
#if LWIP_IPV6
    struct netif* netif = mlua_check_NETIF(ls, 1);
    lua_Unsigned idx = luaL_checkinteger(ls, 2);
    if (idx >= LWIP_IPV6_NUM_ADDRESSES) return mlua_lwip_push_err(ls, ERR_ARG);
    ip6_addr_t const* ip = NULL;
    if (!lua_isnoneornil(ls, 3)) ip = ip_2_ip6(mlua_check_IPAddr6(ls, 3));
    lua_Integer state = luaL_optinteger(ls, 4, -1);
#if LWIP_IPV6_ADDRESS_LIFETIMES
    bool has_valid = !lua_isnoneornil(ls, 5);
    u32_t valid = luaL_optinteger(ls, 5, -1);
    bool has_pref = !lua_isnoneornil(ls, 6);
    u32_t pref = luaL_optinteger(ls, 6, -1);
#endif
    mlua_lwip_lock();
    if (ip != NULL) netif_ip6_addr_set(netif, idx, ip);
    if (state >= 0) netif_ip6_addr_set_state(netif, idx, state);
#if LWIP_IPV6_ADDRESS_LIFETIMES
    if (has_valid) netif_ip6_addr_set_valid_life(netif, idx, valid);
    if (has_pref) netif_ip6_addr_set_pref_life(netif, idx, pref);
#endif
    mlua_lwip_unlock();
    return lua_pushboolean(ls, true), 1;
#else
    return mlua_lwip_push_err(ls, ERR_ARG);
#endif
}

static int NETIF_add_ip6(lua_State* ls) {
#if LWIP_IPV6
    struct netif* netif = mlua_check_NETIF(ls, 1);
    ip_addr_t const* addr = mlua_check_IPAddr(ls, 2);
    luaL_argexpected(ls, IP_IS_V6(addr), 2, "IPv6 address");
    ip6_addr_t const* ip = ip_2_ip6(addr);
    s8_t idx;
    mlua_lwip_lock();
    err_t err = netif_add_ip6_address(netif, ip, &idx);
    mlua_lwip_unlock();
    if (err != ERR_OK) return mlua_lwip_push_err(ls, err);
    return lua_pushinteger(ls, idx), 1;
#else
    return mlua_lwip_push_err(ls, ERR_ARG);
#endif
}

static int NETIF_mtu(lua_State* ls) {
    struct netif* netif = mlua_check_NETIF(ls, 1);
    mlua_lwip_lock();
    u16_t mtu = netif->mtu;
#if LWIP_IPV6
    u16_t mtu6 = netif_mtu6(netif);
#endif
    mlua_lwip_unlock();
    lua_pushinteger(ls, mtu);
#if LWIP_IPV6
    lua_pushinteger(ls, mtu6);
#endif
    return LWIP_IPV6 ? 2 : 1;
}

static int NETIF_up(lua_State* ls) {
    struct netif* netif = mlua_check_NETIF(ls, 1);
    bool set = !lua_isnone(ls, 1);
    bool value;
    if (set) value = mlua_to_cbool(ls, 1);
    mlua_lwip_lock();
    bool res = netif_is_up(netif);
    if (set) {
        if (value) netif_set_up(netif); else netif_set_down(netif);
    }
    mlua_lwip_unlock();
    return lua_pushboolean(ls, res), 1;
}

static int NETIF_link_up(lua_State* ls) {
    struct netif* netif = mlua_check_NETIF(ls, 1);
    bool set = !lua_isnone(ls, 1);
    bool value;
    if (set) value = mlua_to_cbool(ls, 1);
    mlua_lwip_lock();
    bool res = netif_is_link_up(netif);
    if (set) {
        if (value) netif_set_link_up(netif); else netif_set_link_down(netif);
    }
    mlua_lwip_unlock();
    return lua_pushboolean(ls, res), 1;
}

MLUA_SYMBOLS(NETIF_syms) = {
    MLUA_SYM_F(index, NETIF_),
    MLUA_SYM_F(name, NETIF_),
    MLUA_SYM_F(flags, NETIF_),
    MLUA_SYM_F(hwaddr, NETIF_),
    MLUA_SYM_F(ip4, NETIF_),
    MLUA_SYM_F(set_ip4, NETIF_),
    MLUA_SYM_F(ip6, NETIF_),
    MLUA_SYM_F(set_ip6, NETIF_),
    MLUA_SYM_F(add_ip6, NETIF_),
    MLUA_SYM_F(mtu, NETIF_),
    MLUA_SYM_F(up, NETIF_),
    MLUA_SYM_F(link_up, NETIF_),
};

static int mod__default(lua_State* ls) {
    bool update = !lua_isnone(ls, 1);
    struct netif* new_def = NULL;
    if (!lua_isnoneornil(ls, 1)) new_def = mlua_check_NETIF(ls, 1);
    mlua_lwip_lock();
    struct netif* netif = netif_default;
    if (update) netif_set_default(new_def);
    mlua_lwip_unlock();
    if (netif == NULL) return 0;
    *new_NETIF(ls) = netif;
    return 1;
}

static int mod_find(lua_State* ls) {
    u8_t index = NETIF_NO_INDEX;
    char const* name = NULL;
    if (lua_isinteger(ls, 1)) index = lua_tointeger(ls, 1);
    else if (lua_isstring(ls, 1)) name = lua_tostring(ls, 1);
    else return luaL_typeerror(ls, 1, "integer or string");
    mlua_lwip_lock();
    struct netif* netif;
    if (name != NULL) netif = netif_find(name);
    else netif = netif_get_by_index(index);
    mlua_lwip_unlock();
    if (netif == NULL) return mlua_lwip_push_err(ls, ERR_ARG);
    *new_NETIF(ls) = netif;
    return 1;
}

static int iter_next(lua_State* ls) {
    u8_t index = lua_isnoneornil(ls, 2) ? 1
                 : netif_get_index(mlua_check_NETIF(ls, 2)) + 1;
    mlua_lwip_lock();
    struct netif* netif = netif_get_by_index(index);
    mlua_lwip_unlock();
    if (netif == NULL) return 0;
    *new_NETIF(ls) = netif;
    return 1;
}

static int mod_iter(lua_State* ls) {
    return lua_pushcfunction(ls, &iter_next), 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(FLAG_UP, integer, NETIF_FLAG_UP),
    MLUA_SYM_V(FLAG_BROADCAST, integer, NETIF_FLAG_BROADCAST),
    MLUA_SYM_V(FLAG_LINK_UP, integer, NETIF_FLAG_LINK_UP),
    MLUA_SYM_V(FLAG_ETHARP, integer, NETIF_FLAG_ETHARP),
    MLUA_SYM_V(FLAG_ETHERNET, integer, NETIF_FLAG_ETHERNET),
    MLUA_SYM_V(FLAG_IGMP, integer, NETIF_FLAG_IGMP),
    MLUA_SYM_V(FLAG_MLD6, integer, NETIF_FLAG_MLD6),
    MLUA_SYM_V(IPV6_NUM_ADDRESSES, integer, LWIP_IPV6_NUM_ADDRESSES),

    MLUA_SYM_F(_default, mod_),
    MLUA_SYM_F(find, mod_),
    MLUA_SYM_F(iter, mod_),
};

MLUA_OPEN_MODULE(lwip.netif) {
    mlua_require(ls, "lwip", false);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the NETIF class.
    mlua_new_class(ls, mlua_NETIF_name, NETIF_syms, mlua_nosyms);
    lua_pop(ls, 1);
    return 1;
}
