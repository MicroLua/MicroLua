// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "lwip/ip_addr.h"

#include "mlua/lwip.h"
#include "mlua/module.h"
#include "mlua/util.h"

static void mod_ANY(lua_State* ls, MLuaSymVal const* value) {
    ip_addr_t* addr = mlua_new_IPAddr(ls);
    ip_addr_set_any(false, addr);
}

static void mod_LOOPBACK(lua_State* ls, MLuaSymVal const* value) {
    ip_addr_t* addr = mlua_new_IPAddr(ls);
    ip_addr_set_loopback(false, addr);
}

static void mod_BROADCAST(lua_State* ls, MLuaSymVal const* value) {
    ip_addr_t* addr = mlua_new_IPAddr(ls);
    ip_2_ip4(addr)->addr = PP_HTONL(IPADDR_BROADCAST);
    IP_SET_TYPE(addr, IPADDR_TYPE_V4);
}

static int mod_network(lua_State* ls) {
    ip4_addr_t const* addr = ip_2_ip4(mlua_check_IPAddr4(ls, 1));
    ip4_addr_t const* mask = ip_2_ip4(mlua_check_IPAddr4(ls, 2));
    ip_addr_t* res = mlua_new_IPAddr(ls);
    ip4_addr_get_network(ip_2_ip4(res), addr, mask);
    IP_SET_TYPE(res, IPADDR_TYPE_V4);
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_P(ANY, mod_),
    MLUA_SYM_P(LOOPBACK, mod_),
    MLUA_SYM_P(BROADCAST, mod_),

    MLUA_SYM_F(network, mod_),
};

MLUA_OPEN_MODULE(lwip.ip4) {
    mlua_require(ls, "lwip", false);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
