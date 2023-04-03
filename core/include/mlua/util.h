#ifndef _MLUA_CORE_UTIL_H
#define _MLUA_CORE_UTIL_H

#include <stdbool.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MLUA_STR(n) #n

#define MLUA_FUNC_0_0(p, n)             \
static int l_ ## n(lua_State* ls) {     \
    p ## n();                           \
    return 0;                           \
}

#define MLUA_FUNC_0_1(p, n, t1)             \
static int l_ ## n(lua_State* ls) {         \
    p ## n(luaL_check ## t1(ls, 1));        \
    return 0;                               \
}

#define MLUA_FUNC_0_2(p, n, t1, t2)                             \
static int l_ ## n(lua_State* ls) {                             \
    p ## n(luaL_check ## t1(ls, 1), luaL_check ## t2(ls, 2));   \
    return 0;                                                   \
}

#define MLUA_FUNC_0_3(p, n, t1, t2, t3)                         \
static int l_ ## n(lua_State* ls) {                             \
    p ## n(luaL_check ## t1(ls, 1), luaL_check ## t2(ls, 2),    \
           luaL_check ## t3(ls, 3));                            \
    return 0;                                                   \
}

#define MLUA_FUNC_0_4(p, n, t1, t2, t3, t4)                     \
static int l_ ## n(lua_State* ls) {                             \
    p ## n(luaL_check ## t1(ls, 1), luaL_check ## t2(ls, 2),    \
           luaL_check ## t3(ls, 3), luaL_check ## t4(ls, 4));   \
    return 0;                                                   \
}

#define MLUA_FUNC_0_5(p, n, t1, t2, t3, t4, t5)                 \
static int l_ ## n(lua_State* ls) {                             \
    p ## n(luaL_check ## t1(ls, 1), luaL_check ## t2(ls, 2),    \
           luaL_check ## t3(ls, 3), luaL_check ## t4(ls, 4),    \
           luaL_check ## t5(ls, 5));                            \
    return 0;                                                   \
}

#define MLUA_FUNC_1_0(p, n, ret)        \
static int l_ ## n(lua_State* ls) {     \
    lua_push ## ret(ls, p ## n());      \
    return 1;                           \
}

#define MLUA_FUNC_1_1(p, n, ret, t1)                        \
static int l_ ## n(lua_State* ls) {                         \
    lua_push ## ret(ls, p ## n(luaL_check ## t1(ls, 1)));   \
    return 1;                                               \
}

bool mlua_check_cbool(lua_State* ls, int arg);
#define luaL_checkcbool mlua_check_cbool

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

#define MLUA_REG(t, n, v) {.name=MLUA_STR(n), .push=mlua_reg_push_ ## t, .t=(v)}

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
