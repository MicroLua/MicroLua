#include "mlua/int64.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "mlua/util.h"

static char const int64_name[] = "int64";

#define IS64INT (((LUA_MAXINTEGER >> 31) >> 31) >= 1)

#define MLUA_MDIG (l_floatatt(MANT_DIG))

bool mlua_int64_fits_number(int64_t value) {
    return
#if ((((INT64_MAX >> (MLUA_MDIG / 4)) >> (MLUA_MDIG / 4)) >> (MLUA_MDIG / 4)) \
     >> (MLUA_MDIG - (3 * (MLUA_MDIG / 4)))) > 0
#define MLUA_MAX_INT64_IN_NUMBER ((uint64_t)1 << MLUA_MDIG)
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
        v = v * base + digit;
        c = *s++;
    } while (isalnum((unsigned char)c));
    while (isspace((unsigned char)c)) c = *s++;
    if (c != '\0') return false;
    *value = (int64_t)(neg ? (uint64_t)0 - v : v);
    return true;
}

bool mlua_test_int64(lua_State* ls, int arg, int64_t* value) {
#if IS64INT
    if (!lua_isinteger(ls, arg)) return false;
    *value = lua_tointeger(ls, arg);
    return true;
#else
    int64_t* v = luaL_testudata(ls, arg, int64_name);
    if (v == NULL) return false;
    *value = *v;
    return true;
#endif
}

int64_t mlua_check_int64(lua_State* ls, int arg) {
#if IS64INT
    return luaL_checkinteger(ls, arg);
#else
    if (lua_isinteger(ls, arg)) return lua_tointeger(ls, arg);
    int64_t v;
    if (mlua_test_int64(ls, arg, &v)) return v;
    return luaL_argerror(ls, arg, "integer or int64 expected");
#endif
}

void mlua_push_int64(lua_State* ls, int64_t value) {
#if IS64INT
    lua_pushinteger(ls, value);
#else
    int64_t* v = lua_newuserdatauv(ls, sizeof(int64_t), 0);
    *v = value;
    luaL_getmetatable(ls, int64_name);
    lua_setmetatable(ls, -2);
#endif
}

static void int64_max(lua_State* ls, MLuaReg const* reg) {
    mlua_push_int64(ls, INT64_MAX);
}

static void int64_min(lua_State* ls, MLuaReg const* reg) {
    mlua_push_int64(ls, INT64_MIN);
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

#if IS64INT
static int int64___eq(lua_State* ls) {
    lua_pushboolean(ls, lua_compare(ls, 1, 2, LUA_OPEQ));
    return 1;
}
#endif  // IS64INT

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

#if !IS64INT

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

#define CMP_OP(n, op, lmod, lcmp, rmod, rcmp) \
static int int64_ ## n(lua_State* ls) { \
    bool res; \
    int64_t v; \
    if (is_float(ls, 1)) { \
        lua_Number lhs = lua_tonumber(ls, 1); \
        int64_t rhs = mlua_check_int64(ls, 2); \
        if (mlua_int64_fits_number(rhs)) { \
            res = lhs op (lua_Number)rhs; \
        } else if (mlua_number_to_int64_ ## lmod(lhs, &v)) { \
            res = v op rhs; \
        } else { \
            res = lcmp op 0; \
        } \
    } else if (is_float(ls, 2)) { \
        int64_t lhs = mlua_check_int64(ls, 1); \
        lua_Number rhs = lua_tonumber(ls, 2); \
        if (mlua_int64_fits_number(lhs)) { \
            res = (lua_Number)lhs op rhs; \
        } else if (mlua_number_to_int64_ ## rmod(rhs, &v)) { \
            res = lhs op v; \
        } else { \
            res = 0 op rcmp; \
        } \
    } else { \
        res = mlua_check_int64(ls, 1) op mlua_check_int64(ls, 2); \
    } \
    lua_pushboolean(ls, res); \
    return 1; \
}

CMP_OP(__eq, ==, eq, 1, eq, 1)
CMP_OP(__lt, <, floor, lhs, ceil, rhs)
CMP_OP(__le, <=, floor, lhs, ceil, rhs)

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

#endif  // !IS64INT

static int int64___call(lua_State* ls) {
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

    default:
        luaL_checkany(ls, 1);
        luaL_pushfail(ls);
        return 1;
    }
    mlua_push_int64(ls, value);
    return 1;
}

#define int64_eq int64___eq

static MLuaReg const int64_regs[] = {
#define MLUA_SYM(n) MLUA_REG_PUSH(n, int64_ ## n)
    MLUA_SYM(max),
    MLUA_SYM(min),
#undef MLUA_SYM
#define MLUA_SYM(n) MLUA_REG(function, n, int64_ ## n)
    MLUA_SYM(ashr),
    MLUA_SYM(eq),
    MLUA_SYM(hex),
    MLUA_SYM(tointeger),
    MLUA_SYM(tonumber),
    MLUA_SYM(ult),
#if !IS64INT
    MLUA_SYM(__add),
    MLUA_SYM(__sub),
    MLUA_SYM(__mul),
    MLUA_SYM(__idiv),
    MLUA_SYM(__mod),
    MLUA_SYM(__unm),
    MLUA_SYM(__div),
    MLUA_SYM(__pow),
    MLUA_SYM(__band),
    MLUA_SYM(__bor),
    MLUA_SYM(__bxor),
    MLUA_SYM(__bnot),
    MLUA_SYM(__shl),
    MLUA_SYM(__shr),
    MLUA_SYM(__eq),
    MLUA_SYM(__lt),
    MLUA_SYM(__le),
    MLUA_SYM(__tostring),
#ifndef LUA_NOCVTN2S
    MLUA_SYM(__concat),
#endif  // LUA_NOCVTN2S
#endif  // !IS64INT
#undef MLUA_SYM
};

static MLuaReg const int64_meta_regs[] = {
#define MLUA_SYM(n) MLUA_REG(function, n, int64_ ## n)
    MLUA_SYM(__call),
#undef MLUA_SYM
};

int luaopen_mlua_int64(lua_State* ls) {
    // Create the int64 class and make it callable.
    mlua_new_class(ls, int64_name, int64_regs);
    mlua_new_table(ls, int64_meta_regs);
    lua_setmetatable(ls, -2);
    return 1;
}
