#ifndef _MLUA_CORE_UTIL_H
#define _MLUA_CORE_UTIL_H

#include <stdbool.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MLUA_STR(n) #n

#define MLUA_ARGS_1(a1) \
    a1(ls, 1)
#define MLUA_ARGS_2(a1, a2) \
    a1(ls, 1), a2(ls, 2)
#define MLUA_ARGS_3(a1, a2, a3) \
    a1(ls, 1), a2(ls, 2), a3(ls, 3)
#define MLUA_ARGS_4(a1, a2, a3, a4) \
    a1(ls, 1), a2(ls, 2), a3(ls, 3), a4(ls, 4)
#define MLUA_ARGS_5(a1, a2, a3, a4, a5) \
    a1(ls, 1), a2(ls, 2), a3(ls, 3), a4(ls, 4), a5(ls, 5)

#define MLUA_FUNC_0(wp, p, n, args)  \
static int wp ## n(lua_State* ls) { p ## n(args); return 0; }
#define MLUA_FUNC_1(wp, p, n, ret, args)  \
static int wp ## n(lua_State* ls) { ret(ls, p ## n(args)); return 1; }

#define MLUA_FUNC_0_0(wp, p, n) \
    MLUA_FUNC_0(wp, p, n,)
#define MLUA_FUNC_0_1(wp, p, n, a1) \
    MLUA_FUNC_0(wp, p, n, MLUA_ARGS_1(a1))
#define MLUA_FUNC_0_2(wp, p, n, a1, a2) \
    MLUA_FUNC_0(wp, p, n, MLUA_ARGS_2(a1, a2))
#define MLUA_FUNC_0_3(wp, p, n, a1, a2, a3) \
    MLUA_FUNC_0(wp, p, n, MLUA_ARGS_3(a1, a2, a3))
#define MLUA_FUNC_0_4(wp, p, n, a1, a2, a3, a4) \
    MLUA_FUNC_0(wp, p, n, MLUA_ARGS_4(a1, a2, a3, a4))
#define MLUA_FUNC_0_5(wp, p, n, a1, a2, a3, a4, a5) \
    MLUA_FUNC_0(wp, p, n, MLUA_ARGS_5(a1, a2, a3, a4, a5))

#define MLUA_FUNC_1_0(wp, p, n, ret) \
    MLUA_FUNC_1(wp, p, n, ret,)
#define MLUA_FUNC_1_1(wp, p, n, ret, a1) \
    MLUA_FUNC_1(wp, p, n, ret, MLUA_ARGS_1(a1))

bool mlua_to_cbool(lua_State* ls, int arg);

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
#define MLUA_REG_PUSH(n, p) {.name=MLUA_STR(n), .push=p}

void mlua_reg_push_boolean(lua_State* ls, mlua_reg const* reg, int nup);
void mlua_reg_push_integer(lua_State* ls, mlua_reg const* reg, int nup);
void mlua_reg_push_number(lua_State* ls, mlua_reg const* reg, int nup);
void mlua_reg_push_string(lua_State* ls, mlua_reg const* reg, int nup);
void mlua_reg_push_function(lua_State* ls, mlua_reg const* reg, int nup);

void mlua_set_fields(lua_State* ls, mlua_reg const* funcs, int nup);

#define mlua_newtable(ls, fields, extra, nup) \
    do { \
        lua_createtable( \
            ls, 0, sizeof(fields) / sizeof((fields)[0]) - 1 + (extra)); \
        mlua_set_fields(ls, (fields), (nup)); \
    } while(0)


#define mlua_newlib(ls, fields, extra, nup) \
    do { \
        luaL_checkversion(ls); \
        mlua_newtable(ls, fields, extra, nup); \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif
