// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "lwip/ip_addr.h"

#include "mlua/lwip.h"
#include "mlua/module.h"
#include "mlua/util.h"

static void mod_ANY(lua_State* ls, MLuaSymVal const* value) {
    ip_addr_t* addr = mlua_new_IPAddr(ls);
    ip_addr_set_any(true, addr);
}

static void mod_LOOPBACK(lua_State* ls, MLuaSymVal const* value) {
    ip_addr_t* addr = mlua_new_IPAddr(ls);
    ip_addr_set_loopback(true, addr);
}

static int mod_is_global(lua_State* ls) {
    ip_addr_t const* addr = mlua_check_IPAddr6(ls, 1);
    return lua_pushboolean(ls, ip6_addr_isglobal(ip_2_ip6(addr))), 1;
}

static int mod_is_sitelocal(lua_State* ls) {
    ip_addr_t const* addr = mlua_check_IPAddr6(ls, 1);
    return lua_pushboolean(ls, ip6_addr_issitelocal(ip_2_ip6(addr))), 1;
}

static int mod_is_uniquelocal(lua_State* ls) {
    ip_addr_t const* addr = mlua_check_IPAddr6(ls, 1);
    return lua_pushboolean(ls, ip6_addr_isuniquelocal(ip_2_ip6(addr))), 1;
}

static int mod_is_ipv4_mapped_ipv6(lua_State* ls) {
    ip_addr_t const* addr = mlua_check_IPAddr6(ls, 1);
    return lua_pushboolean(ls, ip6_addr_isipv4mappedipv6(ip_2_ip6(addr))), 1;
}

static int mod_is_ipv4_compat(lua_State* ls) {
    ip_addr_t const* addr = mlua_check_IPAddr6(ls, 1);
    return lua_pushboolean(ls, ip6_addr_isipv4compat(ip_2_ip6(addr))), 1;
}

static int mod_multicast_scope(lua_State* ls) {
    ip_addr_t const* addr = mlua_check_IPAddr6(ls, 1);
    return lua_pushinteger(ls, ip6_addr_multicast_scope(ip_2_ip6(addr))), 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(ADDR_LIFE_STATIC, integer, IP6_ADDR_LIFE_STATIC),
    MLUA_SYM_V(ADDR_LIFE_INFINITE, integer, IP6_ADDR_LIFE_INFINITE),
    MLUA_SYM_V(ADDR_INVALID, integer, IP6_ADDR_INVALID),
    MLUA_SYM_V(ADDR_TENTATIVE, integer, IP6_ADDR_TENTATIVE),
    MLUA_SYM_V(ADDR_VALID, integer, IP6_ADDR_VALID),
    MLUA_SYM_V(ADDR_PREFERRED, integer, IP6_ADDR_PREFERRED),
    MLUA_SYM_V(ADDR_DEPRECATED, integer, IP6_ADDR_DEPRECATED),
    MLUA_SYM_V(ADDR_DUPLICATED, integer, IP6_ADDR_DUPLICATED),
    MLUA_SYM_V(MULTICAST_SCOPE_RESERVED0, integer, IP6_MULTICAST_SCOPE_RESERVED0),
    MLUA_SYM_V(MULTICAST_SCOPE_INTERFACE_LOCAL, integer, IP6_MULTICAST_SCOPE_INTERFACE_LOCAL),
    MLUA_SYM_V(MULTICAST_SCOPE_LINK_LOCAL, integer, IP6_MULTICAST_SCOPE_LINK_LOCAL),
    MLUA_SYM_V(MULTICAST_SCOPE_RESERVED3, integer, IP6_MULTICAST_SCOPE_RESERVED3),
    MLUA_SYM_V(MULTICAST_SCOPE_ADMIN_LOCAL, integer, IP6_MULTICAST_SCOPE_ADMIN_LOCAL),
    MLUA_SYM_V(MULTICAST_SCOPE_SITE_LOCAL, integer, IP6_MULTICAST_SCOPE_SITE_LOCAL),
    MLUA_SYM_V(MULTICAST_SCOPE_ORGANIZATION_LOCAL, integer, IP6_MULTICAST_SCOPE_ORGANIZATION_LOCAL),
    MLUA_SYM_V(MULTICAST_SCOPE_GLOBAL, integer, IP6_MULTICAST_SCOPE_GLOBAL),
    MLUA_SYM_V(MULTICAST_SCOPE_RESERVEDF, integer, IP6_MULTICAST_SCOPE_RESERVEDF),
    MLUA_SYM_P(ANY, mod_),
    MLUA_SYM_P(LOOPBACK, mod_),

    MLUA_SYM_F(is_global, mod_),
    MLUA_SYM_F(is_sitelocal, mod_),
    MLUA_SYM_F(is_uniquelocal, mod_),
    MLUA_SYM_F(is_ipv4_mapped_ipv6, mod_),
    MLUA_SYM_F(is_ipv4_compat, mod_),
    MLUA_SYM_F(multicast_scope, mod_),
};

MLUA_OPEN_MODULE(lwip.ip6) {
    mlua_require(ls, "lwip", false);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
