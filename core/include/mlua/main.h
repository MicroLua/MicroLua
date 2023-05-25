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

#ifdef __cplusplus
}
#endif

#endif
