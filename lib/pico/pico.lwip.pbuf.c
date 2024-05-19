// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "lwip/pbuf.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/lwip.h"
#include "mlua/module.h"
#include "mlua/util.h"

char const mlua_PBUF_name[] = "pico.lwip.PBUF";

struct pbuf** mlua_new_PBUF(lua_State* ls) {
    struct pbuf** pb = lua_newuserdatauv(ls, sizeof(struct pbuf*), 0);
    *pb = NULL;
    luaL_getmetatable(ls, mlua_PBUF_name);
    lua_setmetatable(ls, -2);
    return pb;
}

static int PBUF_free(lua_State* ls) {
    struct pbuf** pb = luaL_checkudata(ls, 1, mlua_PBUF_name);
    if (*pb != NULL) {
        mlua_lwip_lock();
        pbuf_free(*pb);
        mlua_lwip_unlock();
        *pb = NULL;
    }
    return 0;
}

static int Buffer___buffer(lua_State* ls) {
    struct pbuf* pb = mlua_check_PBUF(ls, 1);
    if (pb == NULL) return 0;
    lua_pushlightuserdata(ls, pb->payload);
    lua_pushinteger(ls, pb->len);
    return 2;
}

MLUA_SYMBOLS(PBUF_syms) = {
    MLUA_SYM_F(free, PBUF_),
};

#define PBUF___close PBUF_free
#define PBUF___gc PBUF_free

MLUA_SYMBOLS_NOHASH(PBUF_syms_nh) = {
    MLUA_SYM_F_NH(__close, PBUF_),
    MLUA_SYM_F_NH(__gc, PBUF_),
    MLUA_SYM_F_NH(__buffer, Buffer_),
};

static int mod_alloc(lua_State* ls) {
    pbuf_layer layer = luaL_checkinteger(ls, 1);
    u16_t length = luaL_checkinteger(ls, 2);
    pbuf_type type = luaL_optinteger(ls, 3, PBUF_RAM);
    struct pbuf** pb = mlua_new_PBUF(ls);
    mlua_lwip_lock();
    *pb = pbuf_alloc(layer, length, type);
    mlua_lwip_unlock();
    return *pb != NULL ? 1 : 0;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(TRANSPORT, integer, PBUF_TRANSPORT),
    MLUA_SYM_V(IP, integer, PBUF_IP),
    MLUA_SYM_V(LINK, integer, PBUF_LINK),
    MLUA_SYM_V(RAW_TX, integer, PBUF_RAW_TX),
    MLUA_SYM_V(RAW, integer, PBUF_RAW),
    MLUA_SYM_V(RAM, integer, PBUF_RAM),
    MLUA_SYM_V(ROM, integer, PBUF_ROM),
    MLUA_SYM_V(REF, integer, PBUF_REF),
    MLUA_SYM_V(POOL, integer, PBUF_POOL),

    MLUA_SYM_F(alloc, mod_),
};

MLUA_OPEN_MODULE(pico.lwip.pbuf) {
    mlua_require(ls, "pico.lwip", false);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the PBUF class.
    mlua_new_class(ls, mlua_PBUF_name, PBUF_syms, PBUF_syms_nh);
    lua_pop(ls, 1);
    return 1;
}
