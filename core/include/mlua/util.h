// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_CORE_UTIL_H
#define _MLUA_CORE_UTIL_H

#include <stdbool.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MLUA_STR(n) #n
#define MLUA_ESTR(n) MLUA_STR(n)

// Return the number of elements in the given array.
#define MLUA_SIZE(a) (sizeof(a) / sizeof((a)[0]))

// MLUA_IS64INT is true iff Lua is configured with 64-bit integers.
#define MLUA_IS64INT (((LUA_MAXINTEGER >> 31) >> 31) >= 1)

// Initialize the functionality contained in this module.
void mlua_util_init(lua_State* ls);

// A continuation that returns its ctx argument.
int mlua_cont_return_ctx(lua_State* ls, int status, lua_KContext ctx);

// Load a module, and optionally keep a reference to it on the stack.
void mlua_require(lua_State* ls, char const* module, bool keep);

// Convert an argument to a boolean according to C rules: nil, false, 0, 0.0 and
// a missing argument are considered false, and everything else is true.
bool mlua_to_cbool(lua_State* ls, int arg);

// Convert an optional argument to a boolean according to C rules: false, 0 and
// 0.0 are considered false, and everything else is true. If the argument is
// absent or nil, return "def".
bool mlua_opt_cbool(lua_State* ls, int arg, bool def);

// Return the given argument as a userdata. Raises an error if the argument is
// not a userdata.
void* mlua_check_userdata(lua_State* ls, int arg);

// Return the given argument as a userdata, or NULL if the argument is nil.
// Raises an error if the argument is not a userdata or nil.
void* mlua_check_userdata_or_nil(lua_State* ls, int arg);

// Push a failure and an error message, and return the number of pushed values.
int mlua_push_fail(lua_State* ls, char const* err);

#ifdef __cplusplus
}
#endif

#endif
