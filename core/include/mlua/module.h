#ifndef _MLUA_CORE_MODULE_H
#define _MLUA_CORE_MODULE_H

#include <stdbool.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Enable hashed symbol tables for modules and classes, based on perfect
// hashing.
#ifndef MLUA_HASH_SYMBOL_TABLES
#define MLUA_HASH_SYMBOL_TABLES 1
#endif

// Enable double-checking hashed symbol lookups against symbol names. Increases
// flash usage significantly.
#ifndef MLUA_SYMBOL_HASH_DEBUG
#define MLUA_SYMBOL_HASH_DEBUG 0
#endif

// Raise an error about argument 2 specifying an undefined symbol. Can be used
// as an __index function for strict tables.
int mlua_index_undefined(lua_State* ls);

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
#define MLUA_FUNC_1_2(wp, p, n, ret, a1, a2) \
    MLUA_FUNC_1(wp, p, n, ret, MLUA_ARGS_2(a1, a2))
#define MLUA_FUNC_1_3(wp, p, n, ret, a1, a2, a3) \
    MLUA_FUNC_1(wp, p, n, ret, MLUA_ARGS_3(a1, a2, a3))
#define MLUA_FUNC_1_4(wp, p, n, ret, a1, a2, a3, a4) \
    MLUA_FUNC_1(wp, p, n, ret, MLUA_ARGS_4(a1, a2, a3, a4))
#define MLUA_FUNC_1_5(wp, p, n, ret, a1, a2, a3, a4, a5) \
    MLUA_FUNC_1(wp, p, n, ret, MLUA_ARGS_5(a1, a2, a3, a4, a5))

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

// Set the given symbols as fields in the table at index 1.
#define mlua_set_fields(ls, fields) \
    mlua_set_fields_((ls), (fields), MLUA_SYMCNT(fields))
void mlua_set_fields_(lua_State* ls, MLuaSym const* fields, int cnt);

// Set the given symbols as fields in the metatable of the value at index 1.
#define mlua_set_meta_fields(ls, fields) \
    mlua_set_meta_fields_((ls), (fields), MLUA_SYMCNT(fields))
void mlua_set_meta_fields_(lua_State* ls, MLuaSym const* fields, int cnt);

// Create a new table and set its fields from the given (unhasned) symbol table.
#define mlua_new_table(ls, narr, fields) \
    mlua_new_table_((ls), (fields), (narr), MLUA_SYMCNT(fields))
void mlua_new_table_(lua_State* ls, MLuaSym const* fields, int narr, int nrec);

// A set of symbol hash parameters.
typedef struct MLuaSymHash {
    uint32_t seed1, seed2;
    uint16_t nkeys, ng;
} MLuaSymHash;

// Define symbol hash parameters for a symbol table.
#define MLUA_SYMBOLS_HASH_PARAMS(n, s1, s2, k, g) \
static MLuaSymHash const __attribute__((__unused__)) \
    n ## _hash = {.seed1 = s1, .seed2 = s2, .nkeys = k, .ng = g};

// Define symbol hash values for a symbol table.
#define MLUA_SYMBOLS_HASH_VALUES(n) \
static uint8_t const __attribute__((__unused__)) n ## _hash_g[]

void mlua_new_class_nohash_(lua_State* ls, char const* name,
                            MLuaSym const* fields, int cnt, bool strict);
void mlua_new_module_hash_(lua_State* ls, MLuaSymH const* fields, int narr,
                           int nrec, MLuaSymHash const* h, uint8_t const* g);
void mlua_new_class_hash_(
    lua_State* ls, char const* name, MLuaSymH const* fields, int cnt,
    MLuaSymHash const* h, uint8_t const* g, bool strict);

// TODO: Add nrec argument to mlua_new_module_*()

// Create a new module and populated it from the given unhashed symbol table.
#define mlua_new_module_nohash(ls, narr, fields) \
    mlua_new_module_nohash_((ls), (fields), (narr), MLUA_SYMCNT(fields))
void mlua_new_module_nohash_(lua_State* ls, MLuaSym const* fields, int narr,
                             int nrec);

// Create a new class and populated it from the given unhashed symbol table.
#define mlua_new_class_nohash(ls, name, fields, strict) \
    mlua_new_class_nohash_((ls), (name), (fields), MLUA_SYMCNT(fields), (strict))

// Create a new module and populated it from the given hashed symbol table.
#define mlua_new_module_hash(ls, narr, fields) \
    mlua_new_module_hash_((ls), (fields), (narr), MLUA_SYMCNT(fields), \
                          &fields ## _hash, fields ## _hash_g)

// Create a new class and populated it from the given hashed symbol table.
#define mlua_new_class_hash(ls, name, fields, strict) \
    mlua_new_class_hash_((ls), (name), (fields), MLUA_SYMCNT(fields), \
                         &fields ## _hash, fields ## _hash_g, (strict))

#if MLUA_HASH_SYMBOL_TABLES

#define MLUA_SYMBOLS MLUA_SYMBOLS_HASH
#define MLUA_SYM_P MLUA_SYM_P_H
#define MLUA_SYM_V MLUA_SYM_V_H
#define MLUA_SYM_F MLUA_SYM_F_H
#define mlua_new_module mlua_new_module_hash
#define mlua_new_class mlua_new_class_hash

#else  // !MLUA_HASH_SYMBOL_TABLES

#define MLUA_SYMBOLS MLUA_SYMBOLS_NOHASH
#define MLUA_SYM_P MLUA_SYM_P_NH
#define MLUA_SYM_V MLUA_SYM_V_NH
#define MLUA_SYM_F MLUA_SYM_F_NH
#define mlua_new_module mlua_new_module_nohash
#define mlua_new_class mlua_new_class_nohash

#endif  // !MLUA_HASH_SYMBOL_TABLES

// A module registry entry.
typedef struct MLuaModule {
    char const* name;
    lua_CFunction open;
} MLuaModule;

// Define a function to open a module with the given name, and register it.
#define MLUA_OPEN_MODULE(n) \
static int mlua_open_module(lua_State* ls); \
static MLuaModule const module \
    __attribute__((__section__("mlua_module_registry"), __used__)) \
    = {.name = #n, .open = &mlua_open_module}; \
static int mlua_open_module(lua_State* ls)

// Register a module open function.
#define MLUA_REGISTER_MODULE(n, fn) \
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
