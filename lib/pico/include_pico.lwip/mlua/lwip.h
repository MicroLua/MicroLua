// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_PICO_CYW43_H
#define _MLUA_LIB_PICO_PICO_CYW43_H

#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "pico/async_context.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

// Push fail and an error code, and return the number of pushed values.
int mlua_lwip_push_err(lua_State* ls, err_t err);

// If err == ERR_OK, push true, otherwise push fail and an error code. Returns
// the number of pushed values.
int mlua_lwip_push_result(lua_State* ls, err_t err);

// Acquire the lwIP lock.
static inline void mlua_lwip_lock(void) {
    async_context_acquire_lock_blocking(mlua_async_context());
}

// Release the lwIP lock.
static inline void mlua_lwip_unlock(void) {
    async_context_release_lock(mlua_async_context());
}

// Get an IPAddr value at the given stack index, or raise an error if the stack
// entry is not an IPAddr userdata.
static inline ip_addr_t* mlua_check_IPAddr(lua_State* ls, int arg) {
    extern char const mlua_IPAddr_name[];
    return luaL_checkudata(ls, arg, mlua_IPAddr_name);
}

// Get a PBUF value at the given stack index, or raise an error if the stack
// entry is not a PBUF userdata.
static inline struct pbuf* mlua_check_PBUF(lua_State* ls, int arg) {
    extern char const mlua_PBUF_name[];
    return *((struct pbuf**)luaL_checkudata(ls, arg, mlua_PBUF_name));
}

#ifdef __cplusplus
}
#endif

#endif
