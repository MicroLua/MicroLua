// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <assert.h>
#include <float.h>
#include <stdalign.h>

#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/util.h"

// TODO: Add support for views into another array or Buffer
// TODO: Add support for stride

#define HAS_LDOUBLE (LDBL_MANT_DIG != DBL_MANT_DIG)

struct Array;
typedef struct Array Array;

// Vtable for Array.
typedef struct ArrayVT {
    void (*get)(lua_State*, Array const* arr, void const*);
    void (*set)(lua_State*, Array const* arr, int, void*);
} ArrayVT;

// A fixed-capacity homogeneous array.
struct Array {
    ArrayVT const* type;
    void* data;
    lua_Integer len;
    lua_Integer cap;
    size_t size;
    uint64_t d64[0];
};

static void get_int8(lua_State* ls, Array const* arr, void const* data) {
    lua_pushinteger(ls, *(int8_t const*)data);
}

static void get_uint8(lua_State* ls, Array const* arr, void const* data) {
    lua_pushinteger(ls, *(uint8_t const*)data);
}

static void set_uint8(lua_State* ls, Array const* arr, int arg, void* data) {
    *(uint8_t*)data = luaL_checkinteger(ls, arg);
}

static ArrayVT const vt_int8 = {.get = &get_int8, .set = &set_uint8};
static ArrayVT const vt_uint8 = {.get = &get_uint8, .set = &set_uint8};

static void get_int16(lua_State* ls, Array const* arr, void const* data) {
    lua_pushinteger(ls, *(int16_t const*)data);
}

static void get_uint16(lua_State* ls, Array const* arr, void const* data) {
    lua_pushinteger(ls, *(uint16_t const*)data);
}

static void set_uint16(lua_State* ls, Array const* arr, int arg, void* data) {
    *(uint16_t*)data = luaL_checkinteger(ls, arg);
}

static ArrayVT const vt_int16 = {.get = &get_int16, .set = &set_uint16};
static ArrayVT const vt_uint16 = {.get = &get_uint16, .set = &set_uint16};

static void get_int32(lua_State* ls, Array const* arr, void const* data) {
    lua_pushinteger(ls, *(int32_t const*)data);
}

static void get_uint32(lua_State* ls, Array const* arr, void const* data) {
    lua_pushinteger(ls, *(uint32_t const*)data);
}

static void set_uint32(lua_State* ls, Array const* arr, int arg, void* data) {
    *(uint32_t*)data = luaL_checkinteger(ls, arg);
}

static ArrayVT const vt_int32 = {.get = &get_int32, .set = &set_uint32};
static ArrayVT const vt_uint32 = {.get = &get_uint32, .set = &set_uint32};

static void get_uint64(lua_State* ls, Array const* arr, void const* data) {
    mlua_push_int64(ls, *(uint64_t const*)data);
}

static void set_uint64(lua_State* ls, Array const* arr, int arg, void* data) {
    *(uint64_t*)data = mlua_check_int64(ls, arg);
}

static ArrayVT const vt_uint64 = {.get = &get_uint64, .set = &set_uint64};

static lua_Unsigned read_uint(lua_State* ls, uint8_t const* data, size_t size) {
    union { int dummy; char little; } const endian = {1};
    lua_Unsigned v = 0;
    for (size_t i = size; i > 0; --i) {
        v <<= 8;
        v |= (lua_Unsigned)data[endian.little ? i - 1 : size - i];
    }
    return v;
}

static uint64_t read_uint64(lua_State* ls, uint8_t const* data, size_t size) {
    union { int dummy; char little; } const endian = {1};
    uint64_t v = 0;
    for (size_t i = size; i > 0; --i) {
        v <<= 8;
        v |= (uint64_t)data[endian.little ? i - 1 : size - i];
    }
    return v;
}

static void get_int(lua_State* ls, Array const* arr, void const* data) {
    size_t size = arr->size;
    if (size <= sizeof(lua_Unsigned)) {
        lua_Unsigned v = read_uint(ls, data, size);
        lua_Unsigned mask = (lua_Unsigned)1u << (size * 8 - 1);
        lua_pushinteger(ls, (v ^ mask) - mask);  // Perform sign extension
        return;
    }
    uint64_t v = read_uint64(ls, data, size);
    uint64_t mask = (uint64_t)1u << (size * 8 - 1);
    mlua_push_int64(ls, (v ^ mask) - mask);
}

static void get_uint(lua_State* ls, Array const* arr, void const* data) {
    size_t size = arr->size;
    if (size <= sizeof(lua_Unsigned)) {
        lua_pushinteger(ls, read_uint(ls, data, size));
        return;
    }
    mlua_push_int64(ls, read_uint64(ls, data, size));
}

static void set_uint(lua_State* ls, Array const* arr, int arg, void* data) {
    union { int dummy; char little; } const endian = {1};
    size_t size = arr->size;
    uint8_t* data8 = data;
    if (size <= sizeof(lua_Unsigned)) {
        lua_Unsigned v = luaL_checkinteger(ls, arg);
        data8[endian.little ? 0 : size - 1] = v;
        for (size_t i = 1; i < size; ++i) {
            v >>= 8;
            data8[endian.little ? i : size - 1 - i] = v;
        }
    }
    uint64_t v = mlua_check_int64(ls, arg);
    data8[endian.little ? 0 : size - 1] = v;
    for (size_t i = 1; i < size; ++i) {
        v >>= 8;
        data8[endian.little ? i : size - 1 - i] = v;
    }
}

static ArrayVT const vt_int = {.get = &get_int, .set = &set_uint};
static ArrayVT const vt_uint = {.get = &get_uint, .set = &set_uint};

static void get_float(lua_State* ls, Array const* arr, void const* data) {
    lua_pushnumber(ls, *(float const*)data);
}

static void set_float(lua_State* ls, Array const* arr, int arg, void* data) {
    *(float*)data = luaL_checknumber(ls, arg);
}

static ArrayVT const vt_float = {.get = &get_float, .set = &set_float};

static void get_double(lua_State* ls, Array const* arr, void const* data) {
    lua_pushnumber(ls, *(double const*)data);
}

static void set_double(lua_State* ls, Array const* arr, int arg, void* data) {
    *(double*)data = luaL_checknumber(ls, arg);
}

static ArrayVT const vt_double = {.get = &get_double, .set = &set_double};

#if HAS_LDOUBLE

static_assert(alignof(long double) == sizeof(long double));

static void get_ldouble(lua_State* ls, Array const* arr, void const* data) {
    lua_pushnumber(ls, *(long double const*)data);
}

static void set_ldouble(lua_State* ls, Array const* arr, int arg, void* data) {
    *(long double*)data = luaL_checknumber(ls, arg);
}

static ArrayVT const vt_ldouble = {.get = &get_ldouble,
                                         .set = &set_ldouble};

#endif  // HAS_LDOUBLE

char const array_name[] = "mlua.Array";

static Array* new_array(lua_State* ls, size_t data_size) {
    Array* arr = lua_newuserdatauv(ls, sizeof(Array) + data_size, 0);
    luaL_getmetatable(ls, array_name);
    lua_setmetatable(ls, -2);
    return arr;
}

static inline Array* check_array(lua_State* ls, int arg) {
    return luaL_checkudata(ls, arg, array_name);
}

static int array_size(lua_State* ls) {
    Array const* arr = check_array(ls, 1);
    return lua_pushinteger(ls, arr->size), 1;
}

static int array_len(lua_State* ls) {
    Array* arr = check_array(ls, 1);
    lua_Integer len = arr->len;
    if (!lua_isnoneornil(ls, 2)) {
        lua_Integer new_len = luaL_checkinteger(ls, 2);
        luaL_argcheck(ls, new_len >= 0 && new_len <= arr->cap, 2,
                      "invalid length");
        arr->len = new_len;
    }
    return lua_pushinteger(ls, len), 1;
}

static int array___len(lua_State* ls) {
    Array const* arr = check_array(ls, 1);
    return lua_pushinteger(ls, arr->len), 1;
}

static int array_cap(lua_State* ls) {
    Array const* arr = check_array(ls, 1);
    return lua_pushinteger(ls, arr->cap), 1;
}

static int array_addr(lua_State* ls) {
    Array const* arr = check_array(ls, 1);
    return mlua_push_intptr(ls, (uintptr_t)arr->data), 1;
}

static int array___eq(lua_State* ls) {
    Array const* arr1 = check_array(ls, 1);
    Array const* arr2 = check_array(ls, 2);
    if (arr1->len != arr2->len) return lua_pushboolean(ls, false), 1;
    size_t s1 = arr1->size, s2 = arr2->size;
    void const* p1 = arr1->data;
    void const* p2 = arr2->data;
    for (lua_Integer i = arr1->len; i > 0; p1 += s1, p2 += s2, --i) {
        arr1->type->get(ls, arr1, p1);
        arr2->type->get(ls, arr2, p2);
        if (!mlua_compare_eq(ls, -2, -1)) return lua_pushboolean(ls, false), 1;
        lua_pop(ls, 2);
    }
    return lua_pushboolean(ls, true), 1;
}

static int array___tostring(lua_State* ls) {
    Array const* arr = check_array(ls, 1);
    if (arr->len == 0) return lua_pushliteral(ls, "{}"), 1;
    luaL_Buffer buf;
    luaL_buffinit(ls, &buf);
    luaL_addchar(&buf, '{');
    size_t s = arr->size;
    void const* p = arr->data;
    for (lua_Integer i = arr->len; i > 0; p += s, --i) {
        arr->type->get(ls, arr, p);
        luaL_tolstring(ls, -1, NULL);
        luaL_addvalue(&buf);
        lua_pop(ls, 1);
        if (i > 1) luaL_addstring(&buf, ", ");
    }
    luaL_addchar(&buf, '}');
    return luaL_pushresult(&buf), 1;
}

static lua_Integer check_offset(lua_State* ls, int arg, Array const* arr) {
    lua_Integer off = luaL_checkinteger(ls, arg);
    return off + (off >= 0 ? -1 : arr->len);
}

static inline lua_Integer opt_offset(lua_State* ls, int arg,
                                     Array const* arr, lua_Integer def) {
    if (lua_isnoneornil(ls, arg)) return def;
    return check_offset(ls, arg, arr);
}

static int array_get(lua_State* ls) {
    Array const* arr = check_array(ls, 1);
    lua_Integer off = check_offset(ls, 2, arr);
    lua_Integer len = luaL_optinteger(ls, 3, 1);
    if (len <= 0) return 0;
    lua_settop(ls, 1);
    if (luai_unlikely(!lua_checkstack(ls, len))) {
        return luaL_error(ls, "too many results");
    }
    size_t s = arr->size;
    void const* p = arr->data + off * s;
    for (lua_Integer end = off + len; off < end; p += s, ++off) {
        if (luai_likely(0 <= off && off < arr->len)) {
            arr->type->get(ls, arr, p);
        } else {
            lua_pushnil(ls);
        }
    }
    return len;
}

static int array_set(lua_State* ls) {
    Array const* arr = check_array(ls, 1);
    lua_Integer off = check_offset(ls, 2, arr);
    int top = lua_gettop(ls);
    luaL_argcheck(ls, off >= 0 && off + top - 2 <= arr->cap, 2,
                  "out of bounds");
    size_t s = arr->size;
    void* p = arr->data + off * s;
    for (int i = 3; i <= top; p += s, ++i) arr->type->set(ls, arr, i, p);
    return lua_settop(ls, 1), 1;
}

static int ipairs_iter(lua_State* ls) {
    Array const* arr = check_array(ls, 1);
    lua_Integer off = luaL_checkinteger(ls, 2);
    if (off >= arr->len) return 0;
    lua_pushinteger(ls, luaL_intop(+, off, 1));
    arr->type->get(ls, arr, arr->data + off * arr->size);
    return 2;
}

static int array_ipairs(lua_State* ls) {
    check_array(ls, 1);
    lua_pushcfunction(ls, &ipairs_iter);
    lua_pushvalue(ls, 1);
    lua_pushinteger(ls, 0);
    return 3;
}

static int array_append(lua_State* ls) {
    Array* arr = check_array(ls, 1);
    int cnt = lua_gettop(ls) - 1;
    lua_Integer new_len = arr->len + cnt;
    if (new_len > arr->cap) return luaL_error(ls, "out of capacity");
    size_t s = arr->size;
    void* p = arr->data + arr->len * s;
    int top = lua_gettop(ls);
    for (int i = 2; i <= top; p += s, ++i) arr->type->set(ls, arr, i, p);
    arr->len = new_len;
    return lua_settop(ls, 1), 1;
}

static int array_fill(lua_State* ls) {
    Array const* arr = check_array(ls, 1);
    lua_Integer off = opt_offset(ls, 3, arr, 0);
    luaL_argcheck(ls, 0 <= off && off <= arr->cap, 3, "out of bounds");
    lua_Integer len = luaL_optinteger(ls, 4, arr->cap - off);
    if (len <= 0) len = 0;
    luaL_argcheck(ls, off + len <= arr->cap, 4, "out of bounds");
    size_t s = arr->size;
    void* p = arr->data + off * s;
    for (; len > 0; p += s, --len) arr->type->set(ls, arr, 2, p);
    return lua_settop(ls, 1), 1;
}

static size_t parse_size(lua_State* ls, char const** fmt, size_t def) {
    if (**fmt < '0' || **fmt > '9') return def;
    size_t size = *(*fmt)++ - '0';
    if (0 < size && size <= sizeof(uint64_t)) return size;
    return luaL_argerror(ls, 1, "integral size out of limits");
}

static inline ArrayVT const* int_vt(size_t size) {
    switch (size) {
    case sizeof(int8_t): return &vt_int8;
    case sizeof(int16_t): return &vt_int16;
    case sizeof(int32_t): return &vt_int32;
    case sizeof(int64_t): return &vt_uint64;
    }
    if (size <= sizeof(int64_t)) return &vt_int;
    return NULL;
}

static inline ArrayVT const* uint_vt(size_t size) {
    switch (size) {
    case sizeof(uint8_t): return &vt_uint8;
    case sizeof(uint16_t): return &vt_uint16;
    case sizeof(uint32_t): return &vt_uint32;
    case sizeof(uint64_t): return &vt_uint64;
    }
    if (size <= sizeof(uint64_t)) return &vt_uint;
    return NULL;
}

static inline ArrayVT const* number_vt(size_t size) {
    switch (size) {
    case sizeof(float): return &vt_float;
    case sizeof(double): return &vt_double;
#if HAS_LDOUBLE
    case sizeof(long double): return &vt_ldouble;
#endif
    default: return NULL;
    }
}

static int array___call(lua_State* ls) {
    lua_remove(ls, 1);  // Remove class
    char const* fmt = luaL_checkstring(ls, 1);
    size_t size;
    ArrayVT const* vt = NULL;
    switch (*fmt++) {
    case 'b': size = sizeof(signed char); vt = int_vt(size); break;
    case 'B': size = sizeof(unsigned char); vt = uint_vt(size); break;
    case 'h': size = sizeof(signed short); vt = int_vt(size); break;
    case 'H': size = sizeof(unsigned short); vt = uint_vt(size); break;
    case 'i':
        size = parse_size(ls, &fmt, sizeof(signed int));
        vt = int_vt(size);
        break;
    case 'I':
        size = parse_size(ls, &fmt, sizeof(unsigned int));
        vt = uint_vt(size);
        break;
    case 'l': size = sizeof(signed long); vt = int_vt(size); break;
    case 'L': size = sizeof(unsigned long); vt = uint_vt(size); break;
    case 'j': size = sizeof(lua_Integer); vt = int_vt(size); break;
    case 'J': size = sizeof(lua_Unsigned); vt = uint_vt(size); break;
    case 'T': size = sizeof(size_t); vt = uint_vt(size); break;
    case 'f': size = sizeof(float); vt = &vt_float; break;
    case 'd': size = sizeof(double); vt = &vt_double; break;
    case 'n': size = sizeof(lua_Number); vt = number_vt(size); break;
    // TODO: Add support for fixed-size strings
    // case 'c':
    }
    if (vt == NULL || *fmt != '\0') {
        return luaL_argerror(ls, 1, "invalid value format");
    }
    lua_Integer len = luaL_checkinteger(ls, 2);
    lua_Integer cap = luaL_optinteger(ls, 3, len);
    luaL_argcheck(ls, cap >= 0 && (lua_Unsigned)cap <= SIZE_MAX / size, 3,
                  "invalid capacity");
    luaL_argcheck(ls, len >= 0 && len <= cap, 2, "invalid length");

    Array* arr = new_array(ls, cap * size);
    arr->type = vt;
    arr->data = arr->d64;
    arr->size = size;
    arr->len = len;
    arr->cap = cap;
    return 1;
}

MLUA_SYMBOLS(array_syms) = {
    MLUA_SYM_F(size, array_),
    MLUA_SYM_F(len, array_),
    MLUA_SYM_F(cap, array_),
    MLUA_SYM_F(addr, array_),
    MLUA_SYM_F(ipairs, array_),
    MLUA_SYM_F(get, array_),
    MLUA_SYM_F(set, array_),
    MLUA_SYM_F(append, array_),
    MLUA_SYM_F(fill, array_),
    // TODO: MLUA_SYM_F(move, array_),
    // TODO: MLUA_SYM_F(read, array_),
    // TODO: MLUA_SYM_F(write, array_),
};

MLUA_SYMBOLS_NOHASH(array_syms_nh) = {
    MLUA_SYM_F_NH(__len, array_),
    MLUA_SYM_F_NH(__eq, array_),
    MLUA_SYM_F_NH(__tostring, array_),
};

MLUA_SYMBOLS_NOHASH(array_meta_syms) = {
    MLUA_SYM_F_NH(__call, array_),
};

MLUA_OPEN_MODULE(mlua.array) {
    // Create the array class.
    mlua_new_class(ls, array_name, array_syms, true);
    mlua_set_fields(ls, array_syms_nh);
    mlua_set_meta_fields(ls, array_meta_syms);
    return 1;
}
