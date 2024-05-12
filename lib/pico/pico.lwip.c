// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <string.h>

#include "lwip/init.h"
#include "lwip/ip_addr.h"
#include "pico/lwip_nosys.h"

#include "mlua/lwip.h"
#include "mlua/module.h"
#include "mlua/util.h"

int mlua_lwip_push_err(lua_State* ls, err_t err) {
    luaL_pushfail(ls);
    lua_pushinteger(ls, err);
    return 2;
}

int mlua_lwip_push_result(lua_State* ls, err_t err) {
    if (luai_likely(err == ERR_OK)) return lua_pushboolean(ls, true), 1;
    return mlua_lwip_push_err(ls, err);
}

char const mlua_IPAddr_name[] = "pico.lwip.IPAddr";

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
};

MLUA_SYMBOLS_NOHASH(IPAddr_syms_nh) = {
    MLUA_SYM_F_NH(__tostring, IPAddr_),
};


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

static int mod_ipaddr_aton(lua_State* ls) {
    char const* addr = luaL_checkstring(ls, 1);
    ip_addr_t* ia = lua_newuserdatauv(ls, sizeof(ip_addr_t), 0);
    luaL_getmetatable(ls, mlua_IPAddr_name);
    lua_setmetatable(ls, -2);
    return ipaddr_aton(addr, ia) != 0 ? 1 : 0;
}

#define mod_ipaddr_ntoa IPAddr___tostring

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(VERSION, integer, LWIP_VERSION),
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

    MLUA_SYM_F(init, mod_),
    MLUA_SYM_F(deinit, mod_),
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
