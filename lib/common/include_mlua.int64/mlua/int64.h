// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_MLUA_INT64_H
#define _MLUA_LIB_COMMON_MLUA_INT64_H

#include <stdbool.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"
#include "mlua/platform.h"
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

// Get an int64 value at the given stack index. Returns false iff the stack
// entry is not an int64.
static inline bool mlua_test_int64(lua_State* ls, int arg, int64_t* value) {
#if MLUA_IS64INT
    if (luai_unlikely(!lua_isinteger(ls, arg))) return false;
    *value = lua_tointeger(ls, arg);
    return true;
#else
    extern char const mlua_int64_name[];
    int64_t* v = luaL_testudata(ls, arg, mlua_int64_name);
    if (luai_unlikely(v == NULL)) return false;
    *value = *v;
    return true;
#endif
}

// Push an int64 to the stack.
#if MLUA_IS64INT
static inline void mlua_push_int64(lua_State* ls, int64_t value) {
    lua_pushinteger(ls, value);
}
#else
void mlua_push_int64(lua_State* ls, int64_t value);
#endif

// Push an integer if the value fits, or an int64 otherwise.
#if MLUA_IS64INT
static inline void mlua_push_minint(lua_State* ls, int64_t value) {
    lua_pushinteger(ls, value);
}
#else
void mlua_push_minint(lua_State* ls, int64_t value);
#endif

// Get an int64 value at the given stack index. The value must be an integer or
// an int64.
#if MLUA_IS64INT
static inline int64_t mlua_to_int64(lua_State* ls, int arg) {
    return lua_tointeger(ls, arg);
}
#else
int64_t mlua_to_int64(lua_State* ls, int arg);
#endif

// Convert the value at the given stack index to an int64. If "success" is
// non-NULL, set it to true iff the conversion is successful.
#if MLUA_IS64INT
static inline int64_t mlua_to_int64x(lua_State* ls, int arg, int* success) {
    return lua_tointegerx(ls, arg, success);
}
#else
int64_t mlua_to_int64x(lua_State* ls, int arg, int* success);
#endif

// Get an int64 value at the given stack index. Raises an error if the stack
// entry is not an integer or an int64.
#if MLUA_IS64INT
static inline int64_t mlua_check_int64(lua_State* ls, int arg) {
    return luaL_checkinteger(ls, arg);
}
#else
int64_t mlua_check_int64(lua_State* ls, int arg);
#endif

// Push a size_t to the stack.
static inline void mlua_push_size(lua_State* ls, size_t value) {
    if (sizeof(value) <= sizeof(lua_Integer)) {
        lua_pushinteger(ls, value);
    } else {
        mlua_push_int64(ls, value);
    }
}

// Return true iff the given argument is an absolute time value.
static inline bool mlua_is_time(lua_State* ls, int arg) {
#if MLUA_IS64INT
    return lua_isinteger(ls, arg);
#else
    extern char const mlua_int64_name[];
    return lua_isinteger(ls, arg)
           || luaL_testudata(ls, arg, mlua_int64_name) != NULL;
#endif
}

// Get an absolute time. The value must be an integer or an int64.
static inline uint64_t mlua_to_time(lua_State* ls, int arg) {
#if MLUA_IS64INT
    return lua_tointeger(ls, arg);
#else
    if (lua_isinteger(ls, arg)) {
        return mlua_to_ticks64(lua_tointeger(ls, arg), mlua_ticks64());
    }
    return *(int64_t*)lua_touserdata(ls, arg);
#endif
}

// Get an absolute time. Raises an error if the argument is not an integer or an
// int64.
#if MLUA_IS64INT
static inline uint64_t mlua_check_time(lua_State* ls, int arg) {
    return luaL_checkinteger(ls, arg);
}
#else
uint64_t mlua_check_time(lua_State* ls, int arg);
#endif

// Push the absolute time corresponding to a timeout value. Pushes an integer
// time if the timeout is small enough, or an int64 time otherwise.
#if MLUA_IS64INT
static inline void mlua_push_deadline(lua_State* ls, uint64_t timeout) {
    lua_pushinteger(ls, mlua_timeout_deadline(mlua_ticks(), timeout));
}
#else
void mlua_push_deadline(lua_State* ls, uint64_t timeout);
#endif

// Return true iff the absolute time at the given index has been reached. The
// argument must be an integer or an int64.
static inline bool mlua_time_reached(lua_State* ls, int arg) {
#if MLUA_IS64INT
    return mlua_ticks_reached(lua_tointeger(ls, arg));
#else
    if (lua_isinteger(ls, arg)) {
        return mlua_ticks_reached(lua_tointeger(ls, arg));
    }
    return mlua_ticks64_reached(*(int64_t*)lua_touserdata(ls, arg));
#endif
}

#ifdef __cplusplus
}
#endif

#endif
