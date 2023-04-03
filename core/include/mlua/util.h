#ifndef _MLUA_CORE_UTIL_H
#define _MLUA_CORE_UTIL_H

#include <stdbool.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STR(n) #n

typedef struct mlua_reg {
    char const* name;
    void (*push)(lua_State*, struct mlua_reg const*, int);
    union {
        bool boolean;
        lua_Integer integer;
        lua_Number number;
        char const* string;
        lua_CFunction function;
    };
} mlua_reg;

#define MLUA_REG(t, n, v) {.name=STR(n), .push=mlua_reg_push_ ## t, .t=(v)}

void mlua_reg_push_bool(lua_State* ls, mlua_reg const* reg, int nup);
void mlua_reg_push_integer(lua_State* ls, mlua_reg const* reg, int nup);
void mlua_reg_push_number(lua_State* ls, mlua_reg const* reg, int nup);
void mlua_reg_push_string(lua_State* ls, mlua_reg const* reg, int nup);
void mlua_reg_push_function(lua_State* ls, mlua_reg const* reg, int nup);

void mlua_set_funcs(lua_State* ls, mlua_reg const* funcs, int nup);

#define mlua_newlib(ls, funcs, extra, nup)                              \
    do {                                                                \
        luaL_checkversion(ls);                                          \
        lua_createtable(                                                \
            ls, 0, sizeof(funcs) / sizeof((funcs)[0]) - 1 + (extra));   \
        mlua_set_funcs(ls, (funcs), (nup));                             \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif
