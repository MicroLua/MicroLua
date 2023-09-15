#ifndef _MLUA_CORE_UTIL_H
#define _MLUA_CORE_UTIL_H

#include <stdbool.h>
#include <stdint.h>

#include "hardware/sync.h"

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

#define MLUA_STR(n) #n
#define MLUA_ESTR(n) MLUA_STR(n)
#define MLUA_SIZE(a) (sizeof(a) / sizeof((a)[0]))

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

// Initialize functionality contained in this module.
void mlua_util_init(lua_State* ls);

// A continuation that returns its ctx argument.
int mlua_cont_return_ctx(lua_State* ls, int status, lua_KContext ctx);

#if LIB_MLUA_MOD_MLUA_EVENT
// Return true iff yielding is enabled.
bool mlua_yield_enabled(void);
#else
__force_inline static bool mlua_yield_enabled(void) { return false; }
#endif

// Load a module, and optionally keep a reference to it on the stack.
void mlua_require(lua_State* ls, char const* module, bool keep);

// Convert an argument to a boolean according to C rules: nil, false, 0 and 0.0
// are considered false, and everything else is true.
bool mlua_to_cbool(lua_State* ls, int arg);

// Return the given argument as a userdata. Raises an error if the argument is
// not a userdata.
void* mlua_check_userdata(lua_State* ls, int arg);

// Return the given argument as a userdata, or NULL if the argument is nil.
// Raises an error if the argument is not a userdata or nil.
void* mlua_check_userdata_or_nil(lua_State* ls, int arg);

// Raise an error about argument 2 specifying an undefined symbol. Can be used
// as an __index function for strict tables.
int mlua_index_undefined(lua_State* ls);

extern spin_lock_t* mlua_lock;

// TODO: Move symbol registration stuff to module.[hc]

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

// Set the given symbols as fields in the table at index 1.
#define mlua_set_fields(ls, fields) \
    mlua_set_fields_((ls), (fields), MLUA_SIZE(fields))
void mlua_set_fields_(lua_State* ls, MLuaSym const* fields, int cnt);

// Set the given symbols as fields in the metatable of the value at index 1.
#define mlua_set_meta_fields(ls, fields) \
    mlua_set_meta_fields_((ls), (fields), MLUA_SIZE(fields))
void mlua_set_meta_fields_(lua_State* ls, MLuaSym const* fields, int cnt);

// Create a new table and set its fields from the given (unhasned) symbol table.
#define mlua_new_table(ls, narr, fields) \
    mlua_new_table_((ls), (fields), (narr), MLUA_SIZE(fields))
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

// Create a new module and populated it from the given unhashed symbol table.
#define mlua_new_module_nohash(ls, narr, fields) \
    mlua_new_module_nohash_((ls), (fields), (narr), MLUA_SIZE(fields))
void mlua_new_module_nohash_(lua_State* ls, MLuaSym const* fields, int narr,
                             int nrec);

// Create a new class and populated it from the given unhashed symbol table.
#define mlua_new_class_nohash(ls, name, fields, strict) \
    mlua_new_class_nohash_((ls), (name), (fields), MLUA_SIZE(fields), (strict))

// Create a new module and populated it from the given hashed symbol table.
#define mlua_new_module_hash(ls, narr, fields) \
    mlua_new_module_hash_((ls), (fields), (narr), MLUA_SIZE(fields), \
                          &fields ## _hash, fields ## _hash_g)

// Create a new class and populated it from the given hashed symbol table.
#define mlua_new_class_hash(ls, name, fields, strict) \
    mlua_new_class_hash_((ls), (name), (fields), MLUA_SIZE(fields), \
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

// Push the thread metatable field with the given name. Returns the type of the
// field, or LUA_TNIL if the metatable doesn't have this field.
int mlua_thread_meta(lua_State* ls, char const* name);

// Start a new thread calling the function at the top of the stack. Pops the
// function from the stack and pushes the thread.
void mlua_thread_start(lua_State* ls);

// Kill the thread at the top of the stack. Pops the thread from the stack.
void mlua_thread_kill(lua_State* ls);

// Return true iff the given thread is alive.
bool mlua_thread_is_alive(lua_State* thread);

#ifdef __cplusplus
}
#endif

#endif
