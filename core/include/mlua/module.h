// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_CORE_MODULE_H
#define _MLUA_CORE_MODULE_H

#include <stdbool.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"
#include "mlua/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

// Enable hashed symbol tables for modules and classes, based on perfect
// hashing.
#ifndef MLUA_HASH_SYMBOL_TABLES
#define MLUA_HASH_SYMBOL_TABLES MLUA_HASH_SYMBOL_TABLES_DEFAULT
#endif

// Enable double-checking hashed symbol lookups against symbol names. Increases
// flash usage significantly.
#ifndef MLUA_SYMBOL_HASH_DEBUG
#define MLUA_SYMBOL_HASH_DEBUG 0
#endif

// Enable memory allocation statistics.
#ifndef MLUA_ALLOC_STATS
#define MLUA_ALLOC_STATS 0
#endif

// Enable thread statistics.
#ifndef MLUA_THREAD_STATS
#define MLUA_THREAD_STATS 0
#endif

// Per-interpreter global state.
typedef struct MLuaGlobal {
#if MLUA_ALLOC_STATS
    size_t alloc_count;     // Number of memory allocation
    size_t alloc_size;      // Sum of all memory allocations
    size_t alloc_used;      // Memory currently used
    size_t alloc_peak;      // Peak memory usage
#endif
#if LIB_MLUA_MOD_MLUA_THREAD && MLUA_THREAD_STATS
    lua_Unsigned thread_dispatches;     // Number of event dispatch cycles
    lua_Unsigned thread_waits;          // Number of event waits
    lua_Unsigned thread_resumes;        // Number of thread resumes
#endif
} MLuaGlobal;

// Return a pointer to the per-interpreter global state.
MLuaGlobal* mlua_global(lua_State* ls);

// Raise an error about argument 2 specifying an undefined symbol. Can be used
// as an __index function for strict tables.
int mlua_index_undefined(lua_State* ls);

// Write a string with optional "%s" placeholders for the parameters to stderr.
void mlua_writestringerror(char const* fmt, ...);

// Write a string with optional "%s" placeholders for the parameters to stderr,
// then abort the program.
__attribute__((noreturn))
void mlua_abort(char const* fmt, ...);

// Call the function in the first upvalue and return its results. If the
// function raises an error, convert the error to a string and extend it with a
// traceback.
int mlua_with_traceback(lua_State* ls);

// Log the error passed as argument with a traceback to the first upvalue, or
// stderr if the upvalue is none or nil, and return the error unchanged. This
// function can be used as a message handler in pcalls.
int mlua_log_error(lua_State* ls);

// The name of a metatable for weak keys.
extern char const mlua_WeakK_name[];

#define MLUA_FUNC_V(wp, p, n, ...)  \
static int wp ## n(lua_State* ls) { p ## n(__VA_ARGS__); return 0; }
#define MLUA_FUNC_R(wp, p, n, ret, ...)  \
static int wp ## n(lua_State* ls) { return ret(ls, p ## n(__VA_ARGS__)), 1; }
#define MLUA_FUNC_S(wp, p, n, ...)  \
static int wp ## n(lua_State* ls) { \
    p ## n(__VA_ARGS__); \
    return lua_settop(ls, 1), 1; \
}

#define MLUA_FUNC_V0(wp, p, n) \
    MLUA_FUNC_V(wp, p, n)
#define MLUA_FUNC_V1(wp, p, n, a1) \
    MLUA_FUNC_V(wp, p, n, a1(ls, 1))
#define MLUA_FUNC_V2(wp, p, n, a1, a2) \
    MLUA_FUNC_V(wp, p, n, a1(ls, 1), a2(ls, 2))
#define MLUA_FUNC_V3(wp, p, n, a1, a2, a3) \
    MLUA_FUNC_V(wp, p, n, a1(ls, 1), a2(ls, 2), a3(ls, 3))
#define MLUA_FUNC_V4(wp, p, n, a1, a2, a3, a4) \
    MLUA_FUNC_V(wp, p, n, a1(ls, 1), a2(ls, 2), a3(ls, 3), a4(ls, 4))
#define MLUA_FUNC_V5(wp, p, n, a1, a2, a3, a4, a5) \
    MLUA_FUNC_V(wp, p, n, a1(ls, 1), a2(ls, 2), a3(ls, 3), a4(ls, 4), a5(ls, 5))
#define MLUA_FUNC_V6(wp, p, n, a1, a2, a3, a4, a5, a6) \
    MLUA_FUNC_V(wp, p, n, a1(ls, 1), a2(ls, 2), a3(ls, 3), a4(ls, 4), \
                a5(ls, 5), a6(ls, 6))

#define MLUA_FUNC_R0(wp, p, n, ret) \
    MLUA_FUNC_R(wp, p, n, ret)
#define MLUA_FUNC_R1(wp, p, n, ret, a1) \
    MLUA_FUNC_R(wp, p, n, ret, a1(ls, 1))
#define MLUA_FUNC_R2(wp, p, n, ret, a1, a2) \
    MLUA_FUNC_R(wp, p, n, ret, a1(ls, 1), a2(ls, 2))
#define MLUA_FUNC_R3(wp, p, n, ret, a1, a2, a3) \
    MLUA_FUNC_R(wp, p, n, ret, a1(ls, 1), a2(ls, 2), a3(ls, 3))
#define MLUA_FUNC_R4(wp, p, n, ret, a1, a2, a3, a4) \
    MLUA_FUNC_R(wp, p, n, ret, a1(ls, 1), a2(ls, 2), a3(ls, 3), a4(ls, 4))
#define MLUA_FUNC_R5(wp, p, n, ret, a1, a2, a3, a4, a5) \
    MLUA_FUNC_R(wp, p, n, ret, a1(ls, 1), a2(ls, 2), a3(ls, 3), a4(ls, 4), \
                a5(ls, 5))
#define MLUA_FUNC_R6(wp, p, n, ret, a1, a2, a3, a4, a5, a6) \
    MLUA_FUNC_R(wp, p, n, ret, a1(ls, 1), a2(ls, 2), a3(ls, 3), a4(ls, 4), \
                a5(ls, 5), a6(ls, 6))

#define MLUA_FUNC_S1(wp, p, n, a1) \
    MLUA_FUNC_S(wp, p, n, a1(ls, 1))
#define MLUA_FUNC_S2(wp, p, n, a1, a2) \
    MLUA_FUNC_S(wp, p, n, a1(ls, 1), a2(ls, 2))
#define MLUA_FUNC_S3(wp, p, n, a1, a2, a3) \
    MLUA_FUNC_S(wp, p, n, a1(ls, 1), a2(ls, 2), a3(ls, 3))
#define MLUA_FUNC_S4(wp, p, n, a1, a2, a3, a4) \
    MLUA_FUNC_S(wp, p, n, a1(ls, 1), a2(ls, 2), a3(ls, 3), a4(ls, 4))
#define MLUA_FUNC_S5(wp, p, n, a1, a2, a3, a4, a5) \
    MLUA_FUNC_S(wp, p, n, a1(ls, 1), a2(ls, 2), a3(ls, 3), a4(ls, 4), a5(ls, 5))
#define MLUA_FUNC_S6(wp, p, n, a1, a2, a3, a4, a5, a6) \
    MLUA_FUNC_S(wp, p, n, a1(ls, 1), a2(ls, 2), a3(ls, 3), a4(ls, 4), \
                a5(ls, 5), a6(ls, 6))

// A Lua symbol value.
typedef struct MLuaSymVal {
    void (*push)(lua_State*, struct MLuaSymVal const*);
    union {
        bool boolean;
        lua_Integer integer;
        lua_Number number;
        char const* string;
        lua_CFunction function;
        void* lightuserdata;
    };
} MLuaSymVal;

// Push values of various types onto the stack.
void mlua_sym_push_boolean(lua_State* ls, MLuaSymVal const* value);
void mlua_sym_push_integer(lua_State* ls, MLuaSymVal const* value);
void mlua_sym_push_number(lua_State* ls, MLuaSymVal const* value);
void mlua_sym_push_string(lua_State* ls, MLuaSymVal const* value);
void mlua_sym_push_function(lua_State* ls, MLuaSymVal const* value);
void mlua_sym_push_lightuserdata(lua_State* ls, MLuaSymVal const* value);

// A Lua symbol (name, value) pair.
typedef struct MLuaSym {
    char const* name;
    MLuaSymVal value;
} MLuaSym;

// Define an unhashed symbol table.
#define MLUA_SYMBOLS_NOHASH(n) static MLuaSym const n[]

// Define a symbol whose value is given by a push function.
#define MLUA_SYM_P_NH(n, p) {.name = #n, .value = {.push = &p ## n}}

// Define a symbol whose value has a specific type
#define MLUA_SYM_V_NH(n, t, v) \
    {.name = #n, .value = {.push = &mlua_sym_push_ ## t, .t = (v)}}

// Define a function symbol, mapping to the function with the symbol name
// prefixed by the given prefix.
#define MLUA_SYM_F_NH(n, p) \
    {.name = #n, \
     .value = {.push = &mlua_sym_push_function, .function = &p ## n}}

// Define a hashed symbol table.
#define MLUA_SYMBOLS_HASH(n) static MLuaSymH const n[]

#if MLUA_SYMBOL_HASH_DEBUG

// When debugging is enabled, symbol names are stored so that they can be
// verified at lookup time.
typedef MLuaSym MLuaSymH;

#define MLUA_SYM_P_H(n, p) {.name = #n, .value = {.push = &p ## n}}
#define MLUA_SYM_V_H(n, t, v) \
    {.name = #n, .value = {.push = &mlua_sym_push_ ## t, .t = (v)}}
#define MLUA_SYM_F_H(n, p) \
    {.name = #n, \
     .value = {.push = &mlua_sym_push_function, .function = &p ## n}}

#else

// When debugging is disabled, lookups will always succeed, and lookups for
// non-existing symbols will return an undefined value. This allows not storing
// the symbol names, which saves a lot of rodata.
typedef MLuaSymVal MLuaSymH;

#define MLUA_SYM_P_H(n, p) {.push = &p ## n}
#define MLUA_SYM_V_H(n, t, v) {.push = &mlua_sym_push_ ## t, .t = (v)}
#define MLUA_SYM_F_H(n, p) \
    {.push = &mlua_sym_push_function, .function = &p ## n}

#endif

#define MLUA_SYMCNT(a) (sizeof(a) / sizeof((a)[0]))

// A symbol hash.
typedef struct MLuaSymHash {
    MLuaSymH const* fields;
    uint8_t const* g;
    uint32_t seed1, seed2;
    uint16_t nkeys, ng;
    uint8_t bits;
} MLuaSymHash;

// Define a symbol hash for a symbol table.
#define MLUA_SYMBOLS_HASH_FN_HASH(n, s1, s2, nk, nb, ngv, ...) \
static uint8_t const n ## _hash_g[] = {__VA_ARGS__}; \
static MLuaSymHash const n ## _hash = { \
    .fields = n, .g = n ## _hash_g, .seed1 = s1, .seed2 = s2, \
    .nkeys = nk, .ng = ngv, .bits = nb, \
};

// An empty symbol table. Useful for classes without metamethods.
MLUA_SYMBOLS_NOHASH(mlua_nosyms) = {};

// TODO: Add nrec argument to mlua_new_module_*()

// Create a new module and populate it from the given unhashed symbol table.
#define mlua_new_module_nohash(ls, narr, fields) \
    mlua_new_module_nohash_((ls), (fields), (narr), MLUA_SYMCNT(fields))
void mlua_new_module_nohash_(lua_State* ls, MLuaSym const* fields, int narr,
                             int nrec);

// Create a new class and populate it from the given unhashed symbol table.
#define mlua_new_class_nohash(ls, name, fields, nh_fields) \
    mlua_new_class_nohash_((ls), (name), (fields), MLUA_SYMCNT(fields), \
                           (nh_fields), MLUA_SYMCNT(nh_fields))
void mlua_new_class_nohash_(lua_State* ls, char const* name,
                            MLuaSym const* fields, int cnt,
                            MLuaSym const* nh_fields, int nh_cnt);

// Create a new module and populate it from the given hashed symbol table.
#define mlua_new_module_hash(ls, narr, fields) \
    mlua_new_module_hash_((ls), (narr), MLUA_SYMCNT(fields), &fields ## _hash)
void mlua_new_module_hash_(lua_State* ls, int narr, int nrec,
                           MLuaSymHash const* h);

// Create a new class and populate it from the given hashed symbol table.
#define mlua_new_class_hash(ls, name, fields, nh_fields) \
    mlua_new_class_hash_((ls), (name), MLUA_SYMCNT(fields), &fields ## _hash, \
                         (nh_fields), MLUA_SYMCNT(nh_fields))
void mlua_new_class_hash_(lua_State* ls, char const* name, int cnt,
                          MLuaSymHash const* h, MLuaSym const* nh_fields,
                          int nh_cnt);

#if MLUA_HASH_SYMBOL_TABLES

#define MLUA_SYMBOLS MLUA_SYMBOLS_HASH
#define MLUA_SYMBOLS_HASH_FN MLUA_SYMBOLS_HASH_FN_HASH
#define MLUA_SYM_P MLUA_SYM_P_H
#define MLUA_SYM_V MLUA_SYM_V_H
#define MLUA_SYM_F MLUA_SYM_F_H
#define mlua_new_module mlua_new_module_hash
#define mlua_new_class mlua_new_class_hash

#else  // !MLUA_HASH_SYMBOL_TABLES

#define MLUA_SYMBOLS MLUA_SYMBOLS_NOHASH
#define MLUA_SYMBOLS_HASH_FN(...)
#define MLUA_SYM_P MLUA_SYM_P_NH
#define MLUA_SYM_V MLUA_SYM_V_NH
#define MLUA_SYM_F MLUA_SYM_F_NH
#define mlua_new_module mlua_new_module_nohash
#define mlua_new_class mlua_new_class_nohash

#endif  // !MLUA_HASH_SYMBOL_TABLES

// Create a new environment for a Lua module and register it in package.loaded.
void mlua_new_lua_module(lua_State* ls, char const* name);

// Set a metaclass on a class. If the class has a __new key, it is copied to the
// __call key of the metaclass. If the class has an __index key, it is copied to
// the metaclass, so that the same symbols can be looked up on the class as on
// instances.
void mlua_set_metaclass(lua_State* ls);

// A module registry entry.
typedef struct MLuaModule {
    char const* name;
    lua_CFunction open;
} MLuaModule;

// Define a function to open a module with the given name, and register it.
#define MLUA_OPEN_MODULE(n) \
MLUA_PLATFORM_REGISTER_MODULE(n); \
static int mlua_open_module(lua_State* ls); \
static MLuaModule const module \
    __attribute__((__section__("mlua_module_registry"), __used__)) \
    = {.name = #n, .open = &mlua_open_module}; \
static int mlua_open_module(lua_State* ls)

// Register a module open function.
#define MLUA_REGISTER_MODULE(n, fn) \
MLUA_PLATFORM_REGISTER_MODULE(n); \
int fn(lua_State* ls); \
static MLuaModule const module \
    __attribute__((__section__("mlua_module_registry"), __used__)) \
    = {.name = #n, .open = fn}

// Populate package.preload with all comiled-in modules.
void mlua_register_modules(lua_State* ls);

#ifdef __cplusplus
}
#endif

#endif
