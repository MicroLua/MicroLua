// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <string.h>

#include "lwip/err.h"
#include "lwip/init.h"
#include "lwip/ip_addr.h"
#include "pico/lwip_nosys.h"

#include "mlua/lwip.h"
#include "mlua/module.h"
#include "mlua/util.h"

// TODO: Set up lwIP as ext/lwip, use 2.2.0 or head, move to mlua.lwip
// TODO: Look at <https://github.com/cesanta/mongoose>

int mlua_lwip_push_err(lua_State* ls, err_t err) {
    luaL_pushfail(ls);
    lua_pushinteger(ls, err);
    return 2;
}

int mlua_lwip_push_result(lua_State* ls, err_t err) {
    if (luai_likely(err == ERR_OK)) return lua_pushboolean(ls, true), 1;
    return mlua_lwip_push_err(ls, err);
}

static char const* mlua_lwip_err_str(err_t err) {
    switch (err) {
    case ERR_OK: return "no error";
    case ERR_MEM: return "out of memory";
    case ERR_BUF: return "buffer error";
    case ERR_TIMEOUT: return "timeout";
    case ERR_RTE: return "routing problem";
    case ERR_INPROGRESS: return "operation in progress";
    case ERR_VAL: return "illegal value";
    case ERR_WOULDBLOCK: return "operation would block";
    case ERR_USE: return "address in use";
    case ERR_ALREADY: return "already connecting";
    case ERR_ISCONN: return "connection already established";
    case ERR_CONN: return "not connected";
    case ERR_IF: return "low-level netif error";
    case ERR_ABRT: return "connection aborted";
    case ERR_RST: return "connection reset";
    case ERR_CLSD: return "connection closed";
    case ERR_ARG: return "illegal argument";
    default: return "unknown error";
    }
}

char const mlua_IPAddr_name[] = "pico.lwip.IPAddr";

ip_addr_t* mlua_new_IPAddr(lua_State* ls) {
    ip_addr_t* ud = lua_newuserdatauv(ls, sizeof(ip_addr_t), 0);
    luaL_getmetatable(ls, mlua_IPAddr_name);
    lua_setmetatable(ls, -2);
    return ud;
}

static int IPAddr_type(lua_State* ls) {
    ip_addr_t const* addr = mlua_check_IPAddr(ls, 1);
    return lua_pushinteger(ls, IP_GET_TYPE(addr)), 1;
}

static int IPAddr_is_any(lua_State* ls) {
    ip_addr_t const* addr = mlua_check_IPAddr(ls, 1);
    return lua_pushboolean(ls, ip_addr_isany(addr)), 1;
}

static int IPAddr_is_multicast(lua_State* ls) {
    ip_addr_t const* addr = mlua_check_IPAddr(ls, 1);
    return lua_pushboolean(ls, ip_addr_ismulticast(addr)), 1;
}

static int IPAddr_is_loopback(lua_State* ls) {
    ip_addr_t const* addr = mlua_check_IPAddr(ls, 1);
    return lua_pushboolean(ls, ip_addr_isloopback(addr)), 1;
}

static int IPAddr_is_linklocal(lua_State* ls) {
    ip_addr_t const* addr = mlua_check_IPAddr(ls, 1);
    return lua_pushboolean(ls, ip_addr_islinklocal(addr)), 1;
}

static int IPAddr___eq(lua_State* ls) {
    ip_addr_t const* lhs = mlua_check_IPAddr(ls, 1);
    ip_addr_t const* rhs = mlua_check_IPAddr(ls, 2);
    return lua_pushboolean(ls, ip_addr_cmp(lhs, rhs)), 1;
}

static int IPAddr___tostring(lua_State* ls) {
    ip_addr_t const* addr = mlua_check_IPAddr(ls, 1);
    luaL_Buffer buf;
    luaL_buffinit(ls, &buf);
    char* res = luaL_buffaddr(&buf);
    if (ipaddr_ntoa_r(addr, res, buf.size) == NULL) {
        res = luaL_prepbuffsize(&buf, IPADDR_STRLEN_MAX + 1);
        if (ipaddr_ntoa_r(addr, res, buf.size) == NULL) {
            return luaL_error(ls, "buffer too small");
        }
    }
    luaL_addsize(&buf, strlen(res));
    return luaL_pushresult(&buf), 1;
}

MLUA_SYMBOLS(IPAddr_syms) = {
    MLUA_SYM_F(type, IPAddr_),
    MLUA_SYM_F(is_any, IPAddr_),
    MLUA_SYM_F(is_multicast, IPAddr_),
    MLUA_SYM_F(is_loopback, IPAddr_),
    MLUA_SYM_F(is_linklocal, IPAddr_),
};

MLUA_SYMBOLS_NOHASH(IPAddr_syms_nh) = {
    MLUA_SYM_F_NH(__eq, IPAddr_),
    MLUA_SYM_F_NH(__tostring, IPAddr_),
};

static void mod_IP_ANY_TYPE(lua_State* ls, MLuaSymVal const* value) {
    *mlua_new_IPAddr(ls) = *IP_ANY_TYPE;
}

static int mod_init(lua_State* ls) {
    async_context_t* ctx = mlua_async_context();
    mlua_lwip_lock();
    bool res = lwip_nosys_init(ctx);
    if (!res) lwip_nosys_deinit(ctx);
    mlua_lwip_unlock();
    return lua_pushboolean(ls, res), 1;
}

static int mod_deinit(lua_State* ls) {
    async_context_t* ctx = mlua_async_context();
    mlua_lwip_lock();
    lwip_nosys_deinit(ctx);
    mlua_lwip_unlock();
    return 0;
}

static int mod_err_str(lua_State* ls) {
    char const* msg = mlua_lwip_err_str(luaL_checkinteger(ls, 1));
    return lua_pushstring(ls, msg), 1;
}

static int mod_assert(lua_State* ls) {
    if (luai_likely(lua_toboolean(ls, 1))) return lua_gettop(ls);
    lua_settop(ls, 2);
    int ok;
    lua_Integer err = lua_tointegerx(ls, 2, &ok);
    if (ok) lua_pushstring(ls, mlua_lwip_err_str(err));
    return lua_error(ls);
}

static int mod_ipaddr_aton(lua_State* ls) {
    char const* addr = luaL_checkstring(ls, 1);
    ip_addr_t* ia = mlua_new_IPAddr(ls);
    return ipaddr_aton(addr, ia) != 0 ? 1 : 0;
}

#define mod_ipaddr_ntoa IPAddr___tostring

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(VERSION, integer, LWIP_VERSION),
    MLUA_SYM_V(VERSION_STRING, string, LWIP_VERSION_STRING),
    MLUA_SYM_V(IPV4, boolean, LWIP_IPV4),
    MLUA_SYM_V(IPV6, boolean, LWIP_IPV6),
    MLUA_SYM_V(ERR_OK, integer, ERR_OK),
    MLUA_SYM_V(ERR_MEM, integer, ERR_MEM),
    MLUA_SYM_V(ERR_BUF, integer, ERR_BUF),
    MLUA_SYM_V(ERR_TIMEOUT, integer, ERR_TIMEOUT),
    MLUA_SYM_V(ERR_RTE, integer, ERR_RTE),
    MLUA_SYM_V(ERR_INPROGRESS, integer, ERR_INPROGRESS),
    MLUA_SYM_V(ERR_VAL, integer, ERR_VAL),
    MLUA_SYM_V(ERR_WOULDBLOCK, integer, ERR_WOULDBLOCK),
    MLUA_SYM_V(ERR_USE, integer, ERR_USE),
    MLUA_SYM_V(ERR_ALREADY, integer, ERR_ALREADY),
    MLUA_SYM_V(ERR_ISCONN, integer, ERR_ISCONN),
    MLUA_SYM_V(ERR_CONN, integer, ERR_CONN),
    MLUA_SYM_V(ERR_IF, integer, ERR_IF),
    MLUA_SYM_V(ERR_ABRT, integer, ERR_ABRT),
    MLUA_SYM_V(ERR_RST, integer, ERR_RST),
    MLUA_SYM_V(ERR_CLSD, integer, ERR_CLSD),
    MLUA_SYM_V(ERR_ARG, integer, ERR_ARG),
    MLUA_SYM_V(IPADDR_TYPE_V4, integer, IPADDR_TYPE_V4),
    MLUA_SYM_V(IPADDR_TYPE_V6, integer, IPADDR_TYPE_V6),
    MLUA_SYM_V(IPADDR_TYPE_ANY, integer, IPADDR_TYPE_ANY),
    MLUA_SYM_P(IP_ANY_TYPE, mod_),

    MLUA_SYM_F(init, mod_),
    MLUA_SYM_F(deinit, mod_),
    MLUA_SYM_F(err_str, mod_),
    MLUA_SYM_F(assert, mod_),
    MLUA_SYM_F(ipaddr_aton, mod_),
    MLUA_SYM_F(ipaddr_ntoa, mod_),
};

MLUA_OPEN_MODULE(pico.lwip) {
    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the IPAddr class.
    mlua_new_class(ls, mlua_IPAddr_name, IPAddr_syms, IPAddr_syms_nh);
    lua_pop(ls, 1);
    return 1;
}
