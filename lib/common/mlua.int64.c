// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/int64.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "mlua/module.h"

char const mlua_int64_name[] = "mlua.Int64";

#define MLUA_MDIG (l_floatatt(MANT_DIG))

bool mlua_int64_fits_number(int64_t value) {
    return
#if ((((INT64_MAX >> (MLUA_MDIG / 4)) >> (MLUA_MDIG / 4)) >> (MLUA_MDIG / 4)) \
     >> (MLUA_MDIG - (3 * (MLUA_MDIG / 4)))) > 0
#define MLUA_MAX_INT64_IN_NUMBER ((uint64_t)1u << MLUA_MDIG)
        (MLUA_MAX_INT64_IN_NUMBER + (uint64_t)value)
                <= (2 * MLUA_MAX_INT64_IN_NUMBER);
#else
        true;
#endif
}

bool mlua_number_fits_int64(lua_Number num) {
    return (lua_Number)INT64_MIN <= num && num < -(lua_Number)INT64_MIN;
}

bool mlua_number_to_int64(lua_Number num, int64_t* value) {
    if (!mlua_number_fits_int64(num)) return false;
    *value = (int64_t)num;
    return true;
}

bool mlua_number_to_int64_eq(lua_Number num, int64_t* value) {
    if (l_floor(num) != num) return false;
    return mlua_number_to_int64(num, value);
}

bool mlua_number_to_int64_floor(lua_Number num, int64_t* value) {
    lua_Number f = l_floor(num);
    return mlua_number_to_int64(f, value);
}

bool mlua_number_to_int64_ceil(lua_Number num, int64_t* value) {
    lua_Number f = l_floor(num);
    if (f != num) f += 1;
    return mlua_number_to_int64(f, value);
}

int mlua_int64_to_string(int64_t value, char* s, size_t size) {
    return snprintf(s, size, "%"PRId64, value);
}

bool mlua_string_to_int64(char const* s, int base, int64_t* value) {
    while (isspace((unsigned char)*s)) ++s;
    char c = *s++;
    bool neg = c == '-';
    if (neg || c == '+') c = *s++;
    if (base == 0) {
        if (c == '0') {
            switch (*s) {
            case 'x': case 'X': base = 16; break;
            case 'o': base = 8; break;
            case 'b': base = 2; break;
            }
            if (base != 0) { s++; c = *s++; }
        }
        if (base == 0) base = 10;
    }
    if (!isalnum((unsigned char)c)) return false;
    uint64_t v = 0;
    do {
        int digit = isdigit((unsigned char)c) ? c - '0'
                    : toupper((unsigned char)c) - 'A' + 10;
        if (digit >= base) return false;
        // TODO: Check for overflow? See l_str2int() in lobject.c
        v = v * base + digit;
        c = *s++;
    } while (isalnum((unsigned char)c));
    while (isspace((unsigned char)c)) c = *s++;
    if (c != '\0') return false;
    *value = (int64_t)(neg ? (uint64_t)0 - v : v);
    return true;
}

#if !MLUA_IS64INT

void mlua_push_int64(lua_State* ls, int64_t value) {
    int64_t* v = lua_newuserdatauv(ls, sizeof(int64_t), 0);
    *v = value;
    luaL_getmetatable(ls, mlua_int64_name);
    lua_setmetatable(ls, -2);
}

void mlua_push_minint(lua_State* ls, int64_t value) {
    if (luai_likely(LUA_MININTEGER <= value && value <= LUA_MAXINTEGER)) {
        lua_pushinteger(ls, (lua_Integer)value);
    } else {
        mlua_push_int64(ls, value);
    }
}

int64_t mlua_to_int64(lua_State* ls, int arg) {
    if (luai_likely(lua_isinteger(ls, arg))) return lua_tointeger(ls, arg);
    return *(int64_t*)lua_touserdata(ls, arg);
}

int64_t mlua_to_int64x(lua_State* ls, int arg, int* success) {
    int64_t v = lua_tointegerx(ls, arg, success);
    if (luai_likely(*success)) return v;
    if (!mlua_test_int64(ls, arg, &v)) return 0;
    if (success != NULL) *success = true;
    return v;
}

int64_t mlua_check_int64(lua_State* ls, int arg) {
    if (luai_likely(lua_isinteger(ls, arg))) return lua_tointeger(ls, arg);
    int64_t v;
    if (mlua_test_int64(ls, arg, &v)) return v;
    return luaL_typeerror(ls, arg, "integer or Int64");
}

uint64_t mlua_check_time(lua_State* ls, int arg) {
    if (luai_likely(lua_isinteger(ls, arg))) {
        return mlua_to_ticks64(lua_tointeger(ls, arg), mlua_ticks64());
    }
    int64_t time;
    if (mlua_test_int64(ls, arg, &time)) return time;
    return luaL_typeerror(ls, arg, "integer or Int64");
}

void mlua_push_deadline(lua_State* ls, uint64_t timeout) {
    if (luai_likely(timeout <= LUA_MAXINTEGER)) {
        lua_pushinteger(ls, mlua_ticks() + (lua_Unsigned)timeout);
    } else {
        mlua_push_int64(ls, mlua_timeout_deadline(mlua_ticks64(), timeout));
    }
}

#endif  // !MLUA_IS64INT

static int int64___new(lua_State* ls) {
    lua_remove(ls, 1);  // Remove class
    int64_t value = 0;
    switch (lua_type(ls, 1)) {
    case LUA_TBOOLEAN:
        value = lua_toboolean(ls, 1) ? 1 : 0;
        break;

    case LUA_TNUMBER:
        if (lua_isinteger(ls, 1)) {
            int top = lua_gettop(ls);
            int const maxargs = (sizeof(int64_t) + sizeof(lua_Integer) - 1)
                                / sizeof(lua_Integer);
            if (top > maxargs)
                return luaL_error(ls, "too many arguments (max: %d)", maxargs);
            uint64_t mask = ~(uint64_t)0;
            for (int index = 1; index <= top; ++index) {
                int64_t part = (int64_t)luaL_checkinteger(ls, index)
                               << (index - 1) * sizeof(lua_Integer) * 8;
                value = (int64_t)(((uint64_t)value & ~mask) | (uint64_t)part);
                mask = sizeof(lua_Integer) < sizeof(int64_t) ?
                       (mask << sizeof(lua_Integer) * 8) : 0;
            }
        } else if (!mlua_number_to_int64_eq(lua_tonumber(ls, 1), &value)) {
            luaL_pushfail(ls);
            return 1;
        }
        break;

    case LUA_TSTRING:
        lua_Integer base = luaL_optinteger(ls, 2, 0);
        luaL_argcheck(ls, base == 0 || (2 <= base && base <= 36), 2,
                      "base must be 0 or between 2 and 36");
        if (!mlua_string_to_int64(lua_tostring(ls, 1), base, &value)) {
            luaL_pushfail(ls);
            return 1;
        }
        break;

    case LUA_TUSERDATA:
        if (mlua_test_int64(ls, 1, &value)) {
            lua_settop(ls, 1);
            return 1;
        }
        __attribute__((fallthrough));

    default:
        luaL_checkany(ls, 1);
        luaL_pushfail(ls);
        return 1;
    }
    mlua_push_int64(ls, value);
    return 1;
}

#define INT64_OP(lhs, op, rhs) ((int64_t)((uint64_t)(lhs) op (uint64_t)(rhs)))

static int64_t shift_left(int64_t lhs, int64_t rhs, bool arith) {
    if (rhs >= 64) return 0;
    if (rhs >= 0) return INT64_OP(lhs, <<, rhs);
    rhs = -rhs;
    if (rhs >= 64) return arith && lhs < 0 ? -1 : 0;
    if (arith) return lhs >> rhs;
    return INT64_OP(lhs, >>, rhs);
}

static int int64_ashr(lua_State* ls) {
    mlua_push_int64(ls, shift_left(mlua_check_int64(ls, 1),
                                   -mlua_check_int64(ls, 2), true));
    return 1;
}

#if MLUA_IS64INT
static int int64___eq(lua_State* ls) {
    lua_pushboolean(ls, lua_compare(ls, 1, 2, LUA_OPEQ));
    return 1;
}
#endif  // MLUA_IS64INT

static int int64_hex(lua_State* ls) {
    int64_t value = mlua_check_int64(ls, 1);
    lua_Integer width = luaL_optinteger(ls, 2, 0);
    luaL_argcheck(ls, 0 <= width && width <= (lua_Integer)(2 * sizeof(int64_t)),
                  2, "width must be between 0 and 16");
    char s[2 * sizeof(int64_t) + 1];
    int len;
    if (width == 0) {
        len = snprintf(s, sizeof(s), "%"PRIx64, value);
    } else {
        len = snprintf(s, sizeof(s), "%0*"PRIx64, (int)width, value);
    }
    lua_pushlstring(ls, s, len);
    return 1;
}

static int int64_tointeger(lua_State* ls) {
    int64_t value = mlua_check_int64(ls, 1);
    lua_Integer v = (lua_Integer)value;
    if ((int64_t)v == value) {
        lua_pushinteger(ls, v);
    } else {
        luaL_pushfail(ls);
    }
    return 1;
}

static int int64_tonumber(lua_State* ls) {
    lua_pushnumber(ls, (lua_Number)mlua_check_int64(ls, 1));
    return 1;
}

static int int64_ult(lua_State* ls) {
    int64_t lhs = mlua_check_int64(ls, 1), rhs = mlua_check_int64(ls, 2);
    lua_pushboolean(ls, (uint64_t)lhs < (uint64_t)rhs);
    return 1;
}

#if !MLUA_IS64INT

static bool is_float(lua_State* ls, int index) {
    return (lua_type(ls, index) == LUA_TNUMBER && !lua_isinteger(ls, index));
}

static void to_float(lua_State* ls, int index) {
    if (!is_float(ls, index)) {
        lua_pushnumber(ls, mlua_check_int64(ls, index));
        lua_replace(ls, index);
    }
}

static bool float_arith(lua_State* ls, int op) {
    if (is_float(ls, 1)) {
        lua_pushnumber(ls, mlua_check_int64(ls, 2));
        lua_replace(ls, 2);
        lua_arith(ls, op);
        return true;
    } else if (is_float(ls, 2)) {
        lua_pushnumber(ls, mlua_check_int64(ls, 1));
        lua_replace(ls, 1);
        lua_arith(ls, op);
        return true;
    }
    return false;
}

#define ARITH_OP(n, op, aop) \
static int int64_ ## n(lua_State* ls) { \
    if (float_arith(ls, LUA_OP ## aop)) return 1; \
    mlua_push_int64(ls, (int64_t)( \
        (uint64_t)mlua_check_int64(ls, 1) \
        op (uint64_t)mlua_check_int64(ls, 2))); \
    return 1; \
}

ARITH_OP(__add, +, ADD)
ARITH_OP(__sub, -, SUB)
ARITH_OP(__mul, *, MUL)

static int int64___idiv(lua_State* ls) {
    if (float_arith(ls, LUA_OPIDIV)) return 1;
    int64_t res, lhs = mlua_check_int64(ls, 1), rhs = mlua_check_int64(ls, 2);
    if (luai_unlikely((uint64_t)rhs + 1llu <= 1llu)) {  // -1 or 0
        if (rhs == 0) return luaL_error(ls, "attempt to divide by zero");
        // rhs == -1; avoid overflow with 0x800... // -1
        res = INT64_OP(0, -, lhs);
    } else {
        res = lhs / rhs;
        // Fix the rounding if result would be a negative non-integer.
        if ((lhs ^ rhs) < 0 && lhs % rhs != 0) res -= 1;
    }
    mlua_push_int64(ls, res);
    return 1;
}

static int int64___mod(lua_State* ls) {
    if (float_arith(ls, LUA_OPMOD)) return 1;
    int64_t res, lhs = mlua_check_int64(ls, 1), rhs = mlua_check_int64(ls, 2);
    if (luai_unlikely((uint64_t)rhs + 1llu <= 1llu)) {  // -1 or 0
        if (rhs == 0) return luaL_error(ls, "attempt to perform 'n%%0'");
        // lhs % -1 == 0; avoid overflow with 0x800... // -1
        res = 0;
    } else {
        res = lhs % rhs;
        // Fix the rounding if result would be a negative non-integer.
        if (res != 0 && (lhs ^ rhs) < 0) res += rhs;
    }
    mlua_push_int64(ls, res);
    return 1;
}

static int int64___unm(lua_State* ls) {
    mlua_push_int64(ls, INT64_OP(0, -, mlua_check_int64(ls, 1)));
    return 1;
}

#define ARITH_FLOAT_OP(n, aop) \
static int int64_ ## n(lua_State* ls) { \
    to_float(ls, 1); \
    to_float(ls, 2); \
    lua_arith(ls, LUA_OP ## aop); \
    return 1; \
}

ARITH_FLOAT_OP(__div, DIV)
ARITH_FLOAT_OP(__pow, POW)

#define BIT_OP(n, op) \
static int int64_ ## n(lua_State* ls) { \
    mlua_push_int64(ls, (int64_t)( \
        (uint64_t)mlua_check_int64(ls, 1) \
        op (uint64_t)mlua_check_int64(ls, 2))); \
    return 1; \
}

BIT_OP(__band, &)
BIT_OP(__bor, |)
BIT_OP(__bxor, ^)

static int int64___bnot(lua_State* ls) {
    mlua_push_int64(ls, INT64_OP(~(uint64_t)0, ^, mlua_check_int64(ls, 1)));
    return 1;
}

static int int64___shl(lua_State* ls) {
    mlua_push_int64(ls, shift_left(mlua_check_int64(ls, 1),
                                   mlua_check_int64(ls, 2), false));
    return 1;
}

static int int64___shr(lua_State* ls) {
    mlua_push_int64(ls, shift_left(mlua_check_int64(ls, 1),
                                   -mlua_check_int64(ls, 2), false));
    return 1;
}

#define CMP_OP(n, op, lop, lmod, lcmp, rmod, rcmp) \
static inline bool cmp_ ## n(lua_State* ls, int arg1, int arg2) { \
    int i1, i2; \
    int64_t lhs = mlua_to_int64x(ls, arg1, &i1); \
    int64_t rhs = mlua_to_int64x(ls, arg2, &i2); \
    if (i1 && i2) return lhs op rhs; \
    if (i1 && is_float(ls, arg2)) { \
        lua_Number n = lua_tonumber(ls, arg2); \
        if (mlua_int64_fits_number(lhs)) return (lua_Number)lhs op n; \
        int64_t v; \
        if (mlua_number_to_int64_ ## rmod(n, &v)) return lhs op v; \
        return 0 op rcmp; \
    } \
    if (i2 && is_float(ls, arg1)) { \
        lua_Number n = lua_tonumber(ls, arg1); \
        if (mlua_int64_fits_number(rhs)) return n op (lua_Number)rhs; \
        int64_t v; \
        if (mlua_number_to_int64_ ## lmod(n, &v)) return v op rhs; \
        return lcmp op 0; \
    } \
    return lua_compare(ls, arg1, arg2, lop); \
} \
\
static int int64___ ## n(lua_State* ls) { \
    return lua_pushboolean(ls, cmp_ ## n(ls, 1, 2)), 1; \
}

CMP_OP(eq, ==, LUA_OPEQ, eq, 1, eq, 1)
CMP_OP(lt, <, LUA_OPLT, floor, lhs, ceil, rhs)
CMP_OP(le, <=, LUA_OPLE, floor, lhs, ceil, rhs)

static int int64___tostring(lua_State* ls) {
    int64_t v = mlua_check_int64(ls, 1);
    char s[MLUA_MAX_INT64_STR_SIZE];
    int len = mlua_int64_to_string(v, s, sizeof(s));
    lua_pushlstring(ls, s, len);
    return 1;
}

#ifndef LUA_NOCVTN2S
static int int64___concat(lua_State* ls) {
    // TODO: This formats 123.0 .. 1 as "123.00001" instead of "123.01"
    for (int i = 1; i <= 2; ++i) {
        int64_t v;
        if (mlua_test_int64(ls, i, &v)) {
            char s[MLUA_MAX_INT64_STR_SIZE];
            int len = mlua_int64_to_string(v, s, sizeof(s));
            lua_pushlstring(ls, s, len);
            lua_replace(ls, i);
        } else {
            lua_tostring(ls, i);
        }
    }
    lua_concat(ls, 2);
    return 1;
}
#endif  // LUA_NOCVTN2S

#endif  // !MLUA_IS64INT

MLUA_SYMBOLS(int64_syms) = {
    MLUA_SYM_F(ashr, int64_),
    MLUA_SYM_F(hex, int64_),
    MLUA_SYM_F(tointeger, int64_),
    MLUA_SYM_F(tonumber, int64_),
    MLUA_SYM_F(ult, int64_),
};

MLUA_SYMBOLS_NOHASH(int64_syms_nh) = {
    MLUA_SYM_V_NH(max, boolean, false),  // Preallocate
    MLUA_SYM_V_NH(min, boolean, false),  // Preallocate
    MLUA_SYM_F_NH(__new, int64_),
#if !MLUA_IS64INT
    MLUA_SYM_F_NH(__add, int64_),
    MLUA_SYM_F_NH(__sub, int64_),
    MLUA_SYM_F_NH(__mul, int64_),
    MLUA_SYM_F_NH(__idiv, int64_),
    MLUA_SYM_F_NH(__mod, int64_),
    MLUA_SYM_F_NH(__unm, int64_),
    MLUA_SYM_F_NH(__div, int64_),
    MLUA_SYM_F_NH(__pow, int64_),
    MLUA_SYM_F_NH(__band, int64_),
    MLUA_SYM_F_NH(__bor, int64_),
    MLUA_SYM_F_NH(__bxor, int64_),
    MLUA_SYM_F_NH(__bnot, int64_),
    MLUA_SYM_F_NH(__shl, int64_),
    MLUA_SYM_F_NH(__shr, int64_),
    MLUA_SYM_F_NH(__eq, int64_),
    MLUA_SYM_F_NH(__lt, int64_),
    MLUA_SYM_F_NH(__le, int64_),
    MLUA_SYM_F_NH(__tostring, int64_),
#ifndef LUA_NOCVTN2S
    MLUA_SYM_F_NH(__concat, int64_),
#endif  // LUA_NOCVTN2S
#endif  // !MLUA_IS64INT
};

MLUA_OPEN_MODULE(mlua.int64) {
    // Create the int64 class.
    mlua_new_class(ls, mlua_int64_name, int64_syms, int64_syms_nh);
    mlua_set_metaclass(ls);
    mlua_push_int64(ls, INT64_MAX);
    lua_setfield(ls, -2, "max");
    mlua_push_int64(ls, INT64_MIN);
    lua_setfield(ls, -2, "min");
    return 1;
}
