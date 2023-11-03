// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_CORE_MAIN_H
#define _MLUA_CORE_MAIN_H

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Write a string with an optional "%s" placeholder for the parameter to stderr.
void mlua_writestringerror(char const* fmt, char const* param);

// Create a new Lua interpreter.
lua_State* mlua_new_interpreter();

// Load the main module and run its main function.
void mlua_run_main(lua_State* ls);

// Run a Lua interpreter with the configured main module and function on core 0.
void mlua_main_core0();

#ifdef __cplusplus
}
#endif

#endif
