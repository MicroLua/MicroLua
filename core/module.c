// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/module.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "lualib.h"
#include "mlua/util.h"

MLuaGlobal* mlua_global(lua_State* ls) {
    void* ud;
    lua_getallocf(ls, &ud);
    return ud;
}

int mlua_index_undefined(lua_State* ls) {
    return luaL_error(ls, "undefined symbol: %s", lua_tostring(ls, 2));
}

static void write_stderr(char const* fmt, va_list ap) {
    int i = 0;
    for (;;) {
        char c = fmt[i];
        if (c == '\0') {
            if (i > 0) fwrite(fmt, sizeof(char), i, stderr);
            fflush(stderr);
            break;
        } else if (c == '%' && fmt[i + 1] == 's') {
            if (i > 0) fwrite(fmt, sizeof(char), i, stderr);
            char const* param = va_arg(ap, char const*);
            int plen = strlen(param);
            if (plen > 0) fwrite(param, sizeof(char), plen, stderr);
            fmt += i + 2;
            i = 0;
        } else {
            ++i;
        }
    }
}

void mlua_writestringerror(char const* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    write_stderr(fmt, ap);
    va_end(ap);
}

__attribute__((noreturn))
void mlua_abort(char const* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    write_stderr(fmt, ap);
    va_end(ap);
    mlua_platform_abort();
}

static int with_traceback_msgh(lua_State* ls) {
    char const* msg = lua_tostring(ls, 1);
    if (msg == NULL) msg = luaL_tolstring(ls, 1, NULL);
    luaL_traceback(ls, ls, msg, 1);
    lua_replace(ls, 1);
    return lua_settop(ls, 1), 1;
}

int mlua_with_traceback(lua_State* ls) {
    lua_pushcfunction(ls, &with_traceback_msgh);
    lua_pushvalue(ls, lua_upvalueindex(1));
    lua_rotate(ls, 1, 2);
    return mlua_pcallk(ls, lua_gettop(ls) - 2, LUA_MULTRET, 1,
                       mlua_cont_return_results, 1);
}

int mlua_log_error(lua_State* ls) {
    char const* msg = lua_tostring(ls, 1);
    if (msg == NULL) msg = luaL_tolstring(ls, 1, NULL);
    luaL_traceback(ls, ls, msg, 1);
    lua_pushfstring(ls, "ERROR: %s\n", lua_tostring(ls, -1));
    if (!lua_isnoneornil(ls, lua_upvalueindex(1))) {
        lua_getfield(ls, lua_upvalueindex(1), "write");
        lua_pushvalue(ls, lua_upvalueindex(1));
        lua_rotate(ls, -3, -1);
        lua_call(ls, 2, 0);
    } else if (lua_getglobal(ls, "stderr") != LUA_TNIL) {
        lua_getfield(ls, -1, "write");
        lua_rotate(ls, -2, 1);
        lua_rotate(ls, -3, -1);
        lua_call(ls, 2, 0);
    } else {
        mlua_writestringerror("%s", lua_tostring(ls, -1));
    }
    return lua_settop(ls, 1), 1;
}

void mlua_sym_push_boolean(lua_State* ls, MLuaSymVal const* value) {
    lua_pushboolean(ls, value->boolean);
}

void mlua_sym_push_integer(lua_State* ls, MLuaSymVal const* value) {
    lua_pushinteger(ls, value->integer);
}

void mlua_sym_push_number(lua_State* ls, MLuaSymVal const* value) {
    lua_pushnumber(ls, value->number);
}

void mlua_sym_push_string(lua_State* ls, MLuaSymVal const* value) {
    lua_pushstring(ls, value->string);
}

void mlua_sym_push_function(lua_State* ls, MLuaSymVal const* value) {
    lua_pushcfunction(ls, value->function);
}

void mlua_sym_push_lightuserdata(lua_State* ls, MLuaSymVal const* value) {
    lua_pushlightuserdata(ls, value->lightuserdata);
}

void new_metatable(lua_State* ls, char const* name, int narr, int nrec) {
    if (luaL_getmetatable(ls, name) != LUA_TNIL) return;
    lua_pop(ls, 1);
    lua_createtable(ls, narr, 1 + nrec);
    lua_pushstring(ls, name);
    lua_setfield(ls, -2, "__name");
    lua_pushvalue(ls, -1);
    lua_setfield(ls, LUA_REGISTRYINDEX, name);
}

#define set_fields(ls, fields) \
    set_fields_((ls), (fields), MLUA_SYMCNT(fields))

void set_fields_(lua_State* ls, MLuaSym const* fields, int cnt) {
    for (; cnt > 0; --cnt, ++fields) {
        MLuaSymVal const* value = &fields->value;
        value->push(ls, value);
        char const* name = fields->name;
        if (name[0] == '_' && name[1] != '_') ++name;
        lua_setfield(ls, -2, name);
    }
}

static char const Strict_name[] = "mlua.Strict";

MLUA_SYMBOLS_NOHASH(Strict_syms) = {
    MLUA_SYM_V_NH(__index, function, &mlua_index_undefined),
};

static char const Module_name[] = "mlua.Module";

void mlua_new_lua_module(lua_State* ls, char const* name) {
    // Create a new module table.
    lua_createtable(ls, 0, 0);
    luaL_getmetatable(ls, Module_name);
    lua_setmetatable(ls, -2);

    // Set it in package.loaded.
    lua_getfield(ls, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    lua_pushvalue(ls, -2);
    lua_setfield(ls, -2, name);
    lua_pop(ls, 1);
}

static int global_try_1(lua_State* ls, int status, lua_KContext ctx);

static int global_try(lua_State* ls) {
    return mlua_pcallk(ls, lua_gettop(ls) - 1, LUA_MULTRET, 0, global_try_1, 0);
}

static int global_try_1(lua_State* ls, int status, lua_KContext ctx) {
    if (status == LUA_OK || status == LUA_YIELD) return lua_gettop(ls);
    luaL_pushfail(ls);
    lua_rotate(ls, -2, 1);
    return 2;
}

static int global_equal(lua_State* ls) {
    return lua_pushboolean(ls, mlua_compare_eq(ls, 1, 2)), 1;
}

static int global_alloc_stats(lua_State* ls) {
    bool reset = mlua_to_cbool(ls, 1);
#if MLUA_ALLOC_STATS
    MLuaGlobal* g = mlua_global(ls);
    lua_pushinteger(ls, g->alloc_count);
    lua_pushinteger(ls, g->alloc_size);
    lua_pushinteger(ls, g->alloc_used);
    lua_pushinteger(ls, g->alloc_peak);
    if (reset) g->alloc_peak = g->alloc_used;
    return 4;
#else
    (void)reset;
    return 0;
#endif
}

static int global_with_traceback(lua_State* ls) {
    lua_settop(ls, 1);
    lua_pushcclosure(ls, &mlua_with_traceback, 1);
    return 1;
}

static int global_log_error_1(lua_State* ls) {
    int args = lua_gettop(ls);
    bool has_stream = !lua_isnoneornil(ls, lua_upvalueindex(2));
    if (has_stream) lua_pushvalue(ls, lua_upvalueindex(2));
    lua_pushcclosure(ls, &mlua_log_error, has_stream ? 1 : 0);
    lua_pushvalue(ls, lua_upvalueindex(1));
    lua_rotate(ls, 1, 2);
    return mlua_pcallk(ls, args, LUA_MULTRET, 1, mlua_cont_return_results, 1);
}

static int global_log_error(lua_State* ls) {
    if (lua_gettop(ls) > 2) lua_settop(ls, 2);
    lua_pushcclosure(ls, &global_log_error_1, lua_isnoneornil(ls, 2) ? 1 : 2);
    return 1;
}

static int index2(lua_State* ls) {
    // TODO: Cache __index2 as an upvalue
    lua_pushliteral(ls, "__index2");
    if (lua_rawget(ls, lua_upvalueindex(1)) == LUA_TNIL) {
        return mlua_index_undefined(ls);
    }
    lua_pushvalue(ls, 1);
    lua_pushvalue(ls, 2);
    return mlua_callk(ls, 2, 1, mlua_cont_return, 1);
}

static int nohash___index(lua_State* ls) {
    // Try the lookup in the class table.
    lua_pushvalue(ls, 2);
    if (lua_rawget(ls, lua_upvalueindex(1)) != LUA_TNIL) return 1;
    lua_pop(ls, 1);

    // Fall back to __index2.
    return index2(ls);
}

void mlua_new_module_nohash_(lua_State* ls, MLuaSym const* fields, int narr,
                             int nrec) {
    lua_createtable(ls, narr, nrec);
    set_fields_(ls, fields, nrec);
    luaL_getmetatable(ls, Strict_name);
    lua_setmetatable(ls, -2);
}

void mlua_new_class_nohash_(lua_State* ls, char const* name,
                            MLuaSym const* fields, int cnt,
                            MLuaSym const* nh_fields, int nh_cnt) {
    new_metatable(ls, name, 0, cnt + nh_cnt + 1);
    set_fields_(ls, fields, cnt);
    set_fields_(ls, nh_fields, nh_cnt);
    lua_pushvalue(ls, -1);
    lua_pushcclosure(ls, &nohash___index, 1);
    lua_setfield(ls, -2, "__index");
}

#define HASH_MULT 0x13

static uint32_t hash(char const* key, uint32_t seed) {
    for (;;) {
        uint32_t c = (uint32_t)*key;
        if (c == 0) return seed & 0x7fffffff;
        seed = (seed ^ c) * HASH_MULT;
        ++key;
    }
}

static uint32_t lookup_g(uint8_t const* g, uint32_t index, int bits) {
    if (bits == 0) return 0;
    uint32_t bi = index * bits;
    g += bi / 8;
    uint32_t value = (uint32_t)g[0];
    int shift = bi % 8;
    int be = bits + shift;
    if (be > 8) {
        value |= ((uint32_t)g[1] << 8);
        if (be > 16) value |= ((uint32_t)g[2] << 16);
    }
    return (value >> shift) & ((1u << bits) - 1);
}

static uint32_t perfect_hash(char const* key, MLuaSymHash const* h) {
    return (lookup_g(h->g, hash(key, h->seed1) % h->ng, h->bits)
            + lookup_g(h->g, hash(key, h->seed2) % h->ng, h->bits)) % h->nkeys;
}

static int hash___index(lua_State* ls) {
    // Try the lookup in the class table.
    lua_pushvalue(ls, 2);
    if (lua_rawget(ls, lua_upvalueindex(1)) != LUA_TNIL) return 1;
    lua_pop(ls, 1);

    // Try the lookup in the hash table.
    MLuaSymHash const* h = lua_touserdata(ls, lua_upvalueindex(2));
    if (h->nkeys > 0 && lua_isstring(ls, 2)) {
        char const* key = lua_tostring(ls, 2);
        MLuaSymH const* field = h->fields;
        uint32_t kh = perfect_hash(key, h);
        field += kh;
#if MLUA_SYMBOL_HASH_DEBUG
        char const* name = field->name;
        if (name[0] == '_' && name[1] != '_') ++name;
        if (strcmp(key, field->name) != 0) {
            return luaL_error(ls, "bad symbol hash: %s -> %d", key, kh);
        }
        MLuaSymVal const* value = &field->value;
        value->push(ls, value);
#else
        field->push(ls, field);
#endif
        return 1;
    }

    // Fall back to __index2.
    return index2(ls);
}

static void set_hash_index(lua_State* ls, int cnt, MLuaSymHash const* h) {
    if (cnt != h->nkeys) {
        luaL_error(ls, "key count mismatch: %d symbols, expected %d", cnt,
                   h->nkeys);
        return;
    }
    lua_pushvalue(ls, -1);
    lua_pushlightuserdata(ls, (void*)h);
    lua_pushcclosure(ls, &hash___index, 2);
    lua_setfield(ls, -2, "__index");
}

void mlua_new_module_hash_(lua_State* ls, int narr, int nrec,
                           MLuaSymHash const* h) {
    lua_createtable(ls, narr, 0);
    lua_createtable(ls, 0, 1);
    set_hash_index(ls, nrec, h);
    lua_setmetatable(ls, -2);
}

void mlua_new_class_hash_(lua_State* ls, char const* name, int cnt,
                          MLuaSymHash const* h, MLuaSym const* nh_fields,
                          int nh_cnt) {
    new_metatable(ls, name, 0, nh_cnt + 1);
    set_fields_(ls, nh_fields, nh_cnt);
    set_hash_index(ls, cnt, h);
}

void mlua_set_metaclass(lua_State* ls) {
    bool has_new = lua_getfield(ls, -1, "__new") != LUA_TNIL;
    bool has_index = lua_getfield(ls, -2, "__index") != LUA_TNIL;
    lua_createtable(ls, 0, has_new + has_index);
    lua_rotate(ls, -3, 1);
    lua_setfield(ls, -3, "__index");
    lua_setfield(ls, -2, "__call");
    lua_setmetatable(ls, -2);
}

extern MLuaModule const __start_mlua_module_registry[];
extern MLuaModule const __stop_mlua_module_registry[];

static int Preload___index(lua_State* ls) {
    char const* name = luaL_checkstring(ls, 2);
    for (MLuaModule const* m = __start_mlua_module_registry;
            m != __stop_mlua_module_registry; ++m) {
        if (strcmp(m->name, name) == 0) {
            return lua_pushcfunction(ls, m->open), 1;
        }
    }
    return 0;
}

static int Preload_next(lua_State* ls) {
    MLuaModule const* m = __start_mlua_module_registry;
    if (!lua_isnil(ls, 2)) {
        char const* name = lua_tostring(ls, 2);
        while (m != __stop_mlua_module_registry) {
            if (strcmp(m->name, name) == 0) {
                ++m;
                break;
            }
            ++m;
        }
    }
    if (m == __stop_mlua_module_registry) return 0;
    lua_pushstring(ls, m->name);
    lua_pushcfunction(ls, m->open);
    return 2;
}

static int Preload___pairs(lua_State* ls) {
    lua_pushcfunction(ls, &Preload_next);
    return 1;
}

static char const Preload_name[] = "mlua.Preload";

MLUA_SYMBOLS_NOHASH(Preload_syms) = {
    MLUA_SYM_F_NH(__index, Preload_),
    MLUA_SYM_F_NH(__pairs, Preload_),
};

static int pointer___add(lua_State* ls) {
    luaL_argexpected(ls, lua_islightuserdata(ls, 1), 1, "pointer");
    void* ptr = lua_touserdata(ls, 1);
    ptrdiff_t off = luaL_checkinteger(ls, 2);
    return lua_pushlightuserdata(ls, ptr + off), 1;
}

static int pointer___sub(lua_State* ls) {
    luaL_argexpected(ls, lua_islightuserdata(ls, 1), 1, "pointer");
    void* ptr = lua_touserdata(ls, 1);
    switch (lua_type(ls, 2)) {
    case LUA_TLIGHTUSERDATA:
        lua_pushinteger(ls, ptr - lua_touserdata(ls, 2));
        break;
    case LUA_TNUMBER: {
        int ok;
        ptrdiff_t off = lua_tointegerx(ls, 2, &ok);
        if (ok) {
            lua_pushlightuserdata(ls, ptr - off);
            break;
        }
        __attribute__((fallthrough));
    }
    default:
        return luaL_typeerror(ls, 2, "pointer or integer");
    }
    return 1;
}

static int pointer___lt(lua_State* ls) {
    luaL_argexpected(ls, lua_islightuserdata(ls, 1), 1, "pointer");
    luaL_argexpected(ls, lua_islightuserdata(ls, 2), 2, "pointer");
    lua_pushboolean(ls, lua_touserdata(ls, 1) < lua_touserdata(ls, 2));
    return 1;
}

static int pointer___le(lua_State* ls) {
    luaL_argexpected(ls, lua_islightuserdata(ls, 1), 1, "pointer");
    luaL_argexpected(ls, lua_islightuserdata(ls, 2), 2, "pointer");
    lua_pushboolean(ls, lua_touserdata(ls, 1) <= lua_touserdata(ls, 2));
    return 1;
}

static int global_pointer(lua_State* ls) {
    lua_pushlightuserdata(ls, (void*)(uintptr_t)luaL_checkinteger(ls, 1));
    return 1;
}

static int pointer___buffer(lua_State* ls) {
    luaL_argexpected(ls, lua_islightuserdata(ls, 1), 1, "pointer");
    return lua_settop(ls, 1), 1;
}

static char const pointer_name[] = "pointer";

MLUA_SYMBOLS_NOHASH(pointer_syms) = {
    MLUA_SYM_F_NH(__add, pointer_),
    MLUA_SYM_F_NH(__sub, pointer_),
    MLUA_SYM_F_NH(__lt, pointer_),
    MLUA_SYM_F_NH(__le, pointer_),
    MLUA_SYM_F_NH(__buffer, pointer_),
};

static int Function___close(lua_State* ls) {
    // Call the function itself, passing through the remaining arguments. This
    // makes to-be-closed functions the equivalent of deferreds.
    return mlua_callk(ls, lua_gettop(ls) - 1, 0, mlua_cont_return, 0);
}

char const mlua_WeakK_name[] = "mlua.WeakK";

void mlua_register_modules(lua_State* ls) {
    // Require library "base".
    luaL_requiref(ls, "_G", luaopen_base, 1);
    lua_pop(ls, 1);  // _G

    // Create the Strict metatable, and set it on _G.
    lua_pushglobaltable(ls);
    new_metatable(ls, Strict_name, 0, MLUA_SYMCNT(Strict_syms));
    set_fields(ls, Strict_syms);
    lua_setmetatable(ls, -2);
    lua_pop(ls, 1);  // _G

    // Create the Module metatable.
    new_metatable(ls, Module_name, 0, 1);
    lua_pushglobaltable(ls);
    lua_setfield(ls, -2, "__index");
    lua_pop(ls, 1);  // Module

    // Create the Preload metatable, and set it on package.preload to load
    // compiled-in modules.
    luaL_requiref(ls, "package", luaopen_package, 0);
    lua_getfield(ls, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    new_metatable(ls, Preload_name, 0, MLUA_SYMCNT(Preload_syms));
    set_fields(ls, Preload_syms);
    lua_setmetatable(ls, -2);
    lua_pop(ls, 1);  // preload

    // Remove unused searchers.
    lua_getfield(ls, -1, "searchers");
    for (lua_Integer i = luaL_len(ls, -1); i > 1; --i) {
        lua_pushnil(ls);
        lua_seti(ls, -2, i);
    }
    lua_pop(ls, 2);  // searchers, package

    // Set a metatable on light userdata.
    lua_pushlightuserdata(ls, NULL);
    new_metatable(ls, pointer_name, 0, MLUA_SYMCNT(pointer_syms));
    set_fields(ls, pointer_syms);
    lua_setmetatable(ls, -2);
    lua_pop(ls, 1);
    lua_pushcfunction(ls, &global_pointer);
    lua_setglobal(ls, "pointer");

    // Set a metatable on functions.
    lua_pushcfunction(ls, &Function___close);  // Any function will do
    lua_createtable(ls, 0, 1);
    lua_pushcfunction(ls, &Function___close);
    lua_setfield(ls, -2, "__close");
    lua_setmetatable(ls, -2);
    lua_pop(ls, 1);

    // Set globals.
    lua_pushstring(ls, LUA_RELEASE);
    lua_setglobal(ls, "_RELEASE");
    lua_pushinteger(ls, LUA_VERSION_NUM);
    lua_setglobal(ls, "_VERSION_NUM");
    lua_pushinteger(ls, LUA_VERSION_RELEASE_NUM);
    lua_setglobal(ls, "_RELEASE_NUM");
    lua_pushcfunction(ls, &global_try);
    lua_setglobal(ls, "try");
    lua_pushcfunction(ls, &global_equal);
    lua_setglobal(ls, "equal");
    lua_pushcfunction(ls, &global_alloc_stats);
    lua_setglobal(ls, "alloc_stats");
    lua_pushcfunction(ls, &global_with_traceback);
    lua_setglobal(ls, "with_traceback");
    lua_pushcfunction(ls, &global_log_error);
    lua_setglobal(ls, "log_error");

    // Create a metatable for weak keys.
    new_metatable(ls, mlua_WeakK_name, 0, 1);
    lua_pushliteral(ls, "__mode");
    lua_pushliteral(ls, "k");
    lua_rawset(ls, -3);
    lua_setglobal(ls, "WeakK");
}
