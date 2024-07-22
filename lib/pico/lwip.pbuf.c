// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <string.h>

#include "lwip/pbuf.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/lwip.h"
#include "mlua/module.h"
#include "mlua/util.h"

char const mlua_PBUF_name[] = "lwip.PBUF";

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

static int PBUF___len(lua_State* ls) {
    struct pbuf* pb = mlua_check_PBUF(ls, 1);
    if (pb == NULL) return 0;
    return lua_pushinteger(ls, pb->tot_len), 1;
}

static void PBUF_read(void* ptr, lua_Unsigned off, lua_Unsigned len,
                      void* dest) {
    mlua_lwip_lock();
    pbuf_copy_partial((struct pbuf*)ptr, dest, len, off);
    mlua_lwip_unlock();
}

static void PBUF_write(void* ptr, lua_Unsigned off, lua_Unsigned len,
                       void const* src) {
    mlua_lwip_lock();
    pbuf_take_at((struct pbuf*)ptr, src, len, off);
    mlua_lwip_unlock();
}

static void PBUF_fill(void* ptr, lua_Unsigned off, lua_Unsigned len,
                      int value) {
    mlua_lwip_lock();
    u16_t poff;
    struct pbuf* pb = pbuf_skip((struct pbuf*)ptr, off, &poff);
    if (pb != NULL && poff > 0) {
        lua_Unsigned size = len;
        if (poff + size > pb->len) size = pb->len - poff;
        memset(pb->payload + poff, value, size);
        len -= size;
        pb = pb->next;
    }
    while (pb != NULL && len > 0) {
        lua_Unsigned size = len;
        if (size > pb->len) size = pb->len;
        memset(pb->payload, value, size);
        len -= size;
        pb = pb->next;
    }
    mlua_lwip_unlock();
}

static lua_Unsigned PBUF_find(void* ptr, lua_Unsigned off, lua_Unsigned len,
                              void const* needle, lua_Unsigned needle_len) {
    mlua_lwip_lock();
    u16_t pos = pbuf_memfind((struct pbuf*)ptr, needle, needle_len, off);
    mlua_lwip_unlock();
    if (pos == 0xffff || pos + needle_len > off + len) return LUA_MAXUNSIGNED;
    return pos;
}

MLuaBufferVt const PBUF_vt = {.read = &PBUF_read, .write = &PBUF_write,
                              .fill = &PBUF_fill, .find = &PBUF_find};

static int PBUF___buffer(lua_State* ls) {
    struct pbuf* pb = mlua_check_PBUF(ls, 1);
    if (pb == NULL) return 0;
    lua_pushlightuserdata(ls, pb);
    lua_pushinteger(ls, pb->tot_len);
    lua_pushlightuserdata(ls, (void*)&PBUF_vt);
    return 3;
}

MLUA_SYMBOLS(PBUF_syms) = {
    MLUA_SYM_F(free, PBUF_),
};

#define PBUF___close PBUF_free
#define PBUF___gc PBUF_free

MLUA_SYMBOLS_NOHASH(PBUF_syms_nh) = {
    MLUA_SYM_F_NH(__close, PBUF_),
    MLUA_SYM_F_NH(__gc, PBUF_),
    MLUA_SYM_F_NH(__len, PBUF_),
    MLUA_SYM_F_NH(__buffer, PBUF_),
};

static int mod_alloc(lua_State* ls) {
    pbuf_layer layer = luaL_checkinteger(ls, 1);
    u16_t size = luaL_checkinteger(ls, 2);
    pbuf_type type = luaL_optinteger(ls, 3, PBUF_RAM);
    struct pbuf** pb = mlua_new_PBUF(ls);
    mlua_lwip_lock();
    *pb = pbuf_alloc(layer, size, type);
    mlua_lwip_unlock();
    if (*pb == NULL) return mlua_lwip_push_err(ls, ERR_MEM);
    return 1;
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

MLUA_OPEN_MODULE(lwip.pbuf) {
    mlua_require(ls, "lwip", false);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the PBUF class.
    mlua_new_class(ls, mlua_PBUF_name, PBUF_syms, PBUF_syms_nh);
    lua_pop(ls, 1);
    return 1;
}
