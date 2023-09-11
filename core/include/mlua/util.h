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

// TODO: Separate switches for modules and classes
#ifndef MLUA_HASH_SYMBOL_TABLES
#define MLUA_HASH_SYMBOL_TABLES 1
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

void mlua_util_init(lua_State* ls);

// A continuation that returns its ctx argument.
int mlua_cont_return_ctx(lua_State* ls, int status, lua_KContext ctx);

#if LIB_MLUA_MOD_MLUA_EVENT
bool mlua_yield_enabled(void);
#else
__force_inline static bool mlua_yield_enabled(void) { return false; }
#endif
void mlua_require(lua_State* ls, char const* module, bool keep);
bool mlua_to_cbool(lua_State* ls, int arg);
void* mlua_check_userdata(lua_State* ls, int arg);
void* mlua_check_userdata_or_nil(lua_State* ls, int arg);
int mlua_index_undefined(lua_State* ls);

extern spin_lock_t* mlua_lock;

// TODO: Remove the "name" field for hashed tables

typedef struct MLuaSym {
    char const* name;
    void (*push)(lua_State*, struct MLuaSym const*);
    union {
        bool boolean;
        lua_Integer integer;
        lua_Number number;
        char const* string;
        lua_CFunction function;
        void* lightuserdata;
    };
} MLuaSym;

#define MLUA_SYMBOLS(n) static MLuaSym const n[]
#define MLUA_SYMBOLS_NOHASH(n) static MLuaSym const n[]
#define MLUA_SYM_P(n, p) {.name = #n, .push = &p ## n}
#define MLUA_SYM_V(n, t, v) {.name = #n, .push = &mlua_sym_push_ ## t, .t = (v)}
#define MLUA_SYM_F(n, p) \
    {.name = #n, .push = &mlua_sym_push_function, .function = &p ## n}

void mlua_sym_push_boolean(lua_State* ls, MLuaSym const* sym);
void mlua_sym_push_integer(lua_State* ls, MLuaSym const* sym);
void mlua_sym_push_number(lua_State* ls, MLuaSym const* sym);
void mlua_sym_push_string(lua_State* ls, MLuaSym const* sym);
void mlua_sym_push_function(lua_State* ls, MLuaSym const* sym);
void mlua_sym_push_lightuserdata(lua_State* ls, MLuaSym const* sym);

#define mlua_set_fields(ls, fields) \
    mlua_set_fields_((ls), (fields), MLUA_SIZE(fields))
void mlua_set_fields_(lua_State* ls, MLuaSym const* fields, int cnt);
#define mlua_set_meta_fields(ls, fields) \
    mlua_set_meta_fields_((ls), (fields), MLUA_SIZE(fields))
void mlua_set_meta_fields_(lua_State* ls, MLuaSym const* fields, int cnt);

#define mlua_new_table(ls, narr, fields) \
    mlua_new_table_((ls), (fields), (narr), MLUA_SIZE(fields))
void mlua_new_table_(lua_State* ls, MLuaSym const* fields, int narr, int nrec);

typedef struct MLuaSymHash {
    uint32_t seed1, seed2;
    uint16_t nkeys, ng;
} MLuaSymHash;

#define MLUA_SYMBOLS_HASH(n, s1, s2, k, g) \
static MLuaSymHash const __attribute__((__unused__)) \
    n ## _hash = {.seed1 = s1, .seed2 = s2, .nkeys = k, .ng = g};
#define MLUA_SYMBOLS_HASH_VALUES(n) \
static uint8_t const __attribute__((__unused__)) n ## _hash_g[]

void mlua_new_module_(lua_State* ls, MLuaSym const* fields, int narr, int nrec);
void mlua_new_class_(lua_State* ls, char const* name, MLuaSym const* fields,
                     int cnt, bool strict);
void mlua_new_hashed_module_(lua_State* ls, MLuaSym const* fields, int narr,
                             int nrec, MLuaSymHash const* h, uint8_t const* g);
void mlua_new_hashed_class_(
    lua_State* ls, char const* name, MLuaSym const* fields, int cnt,
    MLuaSymHash const* h, uint8_t const* g, bool strict);

#define mlua_new_unhashed_module(ls, narr, fields) \
    mlua_new_module_((ls), (fields), (narr), MLUA_SIZE(fields))
#define mlua_new_unhashed_class(ls, name, fields, strict) \
    mlua_new_class_((ls), (name), (fields), MLUA_SIZE(fields), (strict))
#define mlua_new_hashed_module(ls, narr, fields) \
    mlua_new_hashed_module_((ls), (fields), (narr), MLUA_SIZE(fields), \
                            &fields ## _hash, fields ## _hash_g)
#define mlua_new_hashed_class(ls, name, fields, strict) \
    mlua_new_hashed_class_((ls), (name), (fields), MLUA_SIZE(fields), \
                           &fields ## _hash, fields ## _hash_g, (strict))

#if MLUA_HASH_SYMBOL_TABLES
#define mlua_new_module mlua_new_hashed_module
#define mlua_new_class mlua_new_hashed_class
#else  // !MLUA_HASH_SYMBOL_TABLES
#define mlua_new_module mlua_new_unhashed_module
#define mlua_new_class mlua_new_unhashed_class
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
