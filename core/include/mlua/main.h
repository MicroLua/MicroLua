// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_CORE_MAIN_H
#define _MLUA_CORE_MAIN_H

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Create a new Lua interpreter.
lua_State* mlua_new_interpreter(void);

// Free a Lua interpreter.
void mlua_close_interpreter(lua_State* ls);

// Load the main module and run the main function.
int mlua_run_main(lua_State* ls, int nargs, int nres, int msgh);

// Run a Lua interpreter with the configured main module and function on core 0.
int mlua_main_core0(int argc, char* argv[]);

#ifdef __cplusplus
}
#endif

#endif
