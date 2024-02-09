// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_MLUA_INT64_H
#define _MLUA_LIB_COMMON_MLUA_INT64_H

#include <stdbool.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

#ifdef __cplusplus
extern "C" {
#endif

// The maximum size of an int64 represented as a string, including the trailing
// '\0'.
#define MLUA_MAX_INT64_STR_SIZE 21

// Return true iff the given int64 fits a number without loss.
bool mlua_int64_fits_number(int64_t value);

// Return true iff the given number fits in the range of an int64.
bool mlua_number_fits_int64(lua_Number num);

// Cast a number to an int64. Return false if the number is outside of the range
// of an int64.
bool mlua_number_to_int64(lua_Number num, int64_t* value);

// Convert a number to an int64. Return false if the number has a fractional
// part or is outside of the range of an int64.
bool mlua_number_to_int64_eq(lua_Number num, int64_t* value);

// Take the floor of a number and convert it to an int64. Return false if the
// result is outside of the range of an int64.
bool mlua_number_to_int64_floor(lua_Number num, int64_t* value);

// Take the ceiling of a number and convert it to an int64. Return false if the
// result is outside of the range of an int64.
bool mlua_number_to_int64_ceil(lua_Number num, int64_t* value);

// Convert an int64 to a string. Return the length of the result.
int mlua_int64_to_string(int64_t value, char* s, size_t size);

// Convert a string to an int64. If the base is 0, the string can have a '0x',
// '0o' or '0b' prefix to indicate hex, octal or binary, respectively. Return
// false if the conversion fails.
bool mlua_string_to_int64(char const* s, int base, int64_t* value);

// Get an int64 value at the given stack index. Returns false if the stack
// entry is not an int64.
bool mlua_test_int64(lua_State* ls, int arg, int64_t* value);

// Get an int64 value at the given stack index. Raises an error if the stack
// entry is not an integer or an int64.
int64_t mlua_check_int64(lua_State* ls, int arg);

// Get an int64 value at the given stack index. The value must be an integer or
// an int64, otherwise the function returns zero.
int64_t mlua_to_int64(lua_State* ls, int arg);

// Get an int64 value at the given stack index. The value must be an int64.
static inline int64_t mlua_to_int64_strict(lua_State* ls, int arg) {
#if MLUA_IS64INT
    return lua_tointeger(ls, arg);
#else
    int64_t* v = lua_touserdata(ls, arg);
    return v != NULL ? *v : 0;
#endif
}

// Push an int64 to the stack.
void mlua_push_int64(lua_State* ls, int64_t value);

// Push an integer if the value fits, or an int64 otherwise.
void mlua_push_minint(lua_State* ls, int64_t value);

// Get an intptr_t value at the given stack index, or raise an error if the
// stack entry doesn't have the right type.
static inline intptr_t mlua_check_intptr(lua_State* ls, int arg) {
    if (sizeof(intptr_t) <= sizeof(lua_Integer)) {
        return luaL_checkinteger(ls, arg);
    }
    return mlua_check_int64(ls, arg);
}

// Push an intptr_t to the stack.
static inline void mlua_push_intptr(lua_State* ls, intptr_t value) {
    if (sizeof(value) <= sizeof(lua_Integer)) {
        lua_pushinteger(ls, value);
    } else {
        mlua_push_int64(ls, value);
    }
}

#ifdef __cplusplus
}
#endif

#endif
