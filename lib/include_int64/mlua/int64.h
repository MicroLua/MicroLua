#ifndef _MLUA_LIB_INT64_H
#define _MLUA_LIB_INT64_H

#include <stdbool.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MLUA_MAX_INT64_STR_SIZE 21

bool mlua_int64_fits_number(int64_t value);
bool mlua_number_fits_int64(lua_Number num);
bool mlua_number_to_int64(lua_Number num, int64_t* value);
bool mlua_number_to_int64_eq(lua_Number num, int64_t* value);
bool mlua_number_to_int64_floor(lua_Number num, int64_t* value);
bool mlua_number_to_int64_ceil(lua_Number num, int64_t* value);
int mlua_int64_to_string(int64_t value, char* s, size_t size);
bool mlua_string_to_int64(char const* s, int base, int64_t* value);
bool mlua_test_int64(lua_State* ls, int arg, int64_t* value);
int64_t mlua_check_int64(lua_State* ls, int arg);
void mlua_push_int64(lua_State* ls, int64_t value);

#ifdef __cplusplus
}
#endif

#endif
