// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_CORE_MODULE_H
#define _MLUA_CORE_MODULE_H

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// A module registry entry.
typedef struct MLuaModule {
    char const* name;
    lua_CFunction open;
} MLuaModule;

// Define a function to open a module with the given name, and register it.
#define MLUA_OPEN_MODULE(n) \
static int mlua_open_module(lua_State* ls); \
static MLuaModule const module \
    __attribute__((__section__("mlua_module_registry"), __used__)) \
    = {.name = #n, .open = &mlua_open_module}; \
static int mlua_open_module(lua_State* ls)

#ifdef __cplusplus
}
#endif

#endif