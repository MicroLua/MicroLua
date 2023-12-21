// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_MLUA_MEM_H
#define _MLUA_LIB_COMMON_MLUA_MEM_H

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Get a Buffer value, or raise an error if the argument is not a Buffer
// userdata.
static inline void* mlua_mem_check_Buffer(lua_State* ls, int arg) {
    extern char const mlua_Buffer_name[];
    return luaL_checkudata(ls, arg, mlua_Buffer_name);
}

#ifdef __cplusplus
}
#endif

#endif
