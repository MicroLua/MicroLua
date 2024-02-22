// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/module.h"

#include <string.h>

#include "lualib.h"

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

int mlua_index_undefined(lua_State* ls) {
    return luaL_error(ls, "undefined symbol: %s", lua_tostring(ls, 2));
}

void mlua_set_fields_(lua_State* ls, MLuaSym const* fields, int cnt) {
    for (; cnt > 0; --cnt, ++fields) {
        MLuaSymVal const* value = &fields->value;
        value->push(ls, value);
        char const* name = fields->name;
        if (name[0] == '_' && name[1] != '_') ++name;
        lua_setfield(ls, -2, name);
    }
}

void mlua_set_meta_fields_(lua_State* ls, MLuaSym const* fields, int cnt) {
    lua_getmetatable(ls, -1);
    mlua_set_fields_(ls, fields, cnt);
    lua_pop(ls, 1);
}

void mlua_new_table_(lua_State* ls, MLuaSym const* fields, int narr, int nrec) {
    lua_createtable(ls, narr, nrec);
    mlua_set_fields_(ls, fields, nrec);
}

static char const Strict_name[] = "mlua.Strict";

MLUA_SYMBOLS_NOHASH(Strict_syms) = {
    MLUA_SYM_V_NH(__index, function, &mlua_index_undefined),
};

static char const Module_name[] = "mlua.Module";

static int global_module(lua_State* ls) {
    lua_getfield(ls, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);

    // Create a new module table.
    lua_createtable(ls, 0, 0);
    luaL_getmetatable(ls, Module_name);
    lua_setmetatable(ls, -2);

    // Set it in package.loaded.
    lua_pushvalue(ls, 1);
    lua_pushvalue(ls, -2);
    lua_settable(ls, -4);
    return 1;
}

static int global_try(lua_State* ls) {
    if (lua_pcall(ls, lua_gettop(ls) - 1, LUA_MULTRET, 0) == LUA_OK) {
        return lua_gettop(ls);
    }
    luaL_pushfail(ls);
    lua_rotate(ls, -2, 1);
    return 2;
}

void mlua_new_module_nohash_(lua_State* ls, MLuaSym const* fields, int narr,
                             int nrec) {
    mlua_new_table_(ls, fields, narr, nrec);
    luaL_getmetatable(ls, Strict_name);
    lua_setmetatable(ls, -2);
}

void mlua_new_class_nohash_(lua_State* ls, char const* name,
                            MLuaSym const* fields, int cnt, bool strict) {
    luaL_newmetatable(ls, name);
    mlua_set_fields_(ls, fields, cnt);
    lua_pushvalue(ls, -1);
    lua_setfield(ls, -2, "__index");
    lua_createtable(ls, 0, 1);
    if (strict) {
        lua_pushcfunction(ls, &mlua_index_undefined);
        lua_setfield(ls, -2, "__index");
    }
    lua_setmetatable(ls, -2);
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

static uint32_t perfect_hash(char const* key, MLuaSymHash const* h,
                             uint8_t const* g, int bits) {
    return (lookup_g(g, hash(key, h->seed1) % h->ng, bits)
            + lookup_g(g, hash(key, h->seed2) % h->ng, bits)) % h->nkeys;
}

static int hashed_index(lua_State* ls) {
    if (lua_isstring(ls, 2)) {
        MLuaSymHash const* h = lua_touserdata(ls, lua_upvalueindex(2));
        uint8_t const* g = lua_touserdata(ls, lua_upvalueindex(3));
        int bits = lua_tointeger(ls, lua_upvalueindex(4));
        char const* key = lua_tostring(ls, 2);
        MLuaSymH const* field = lua_touserdata(ls, lua_upvalueindex(1));
        uint32_t kh = perfect_hash(key, h, g, bits);
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
    if (lua_toboolean(ls, lua_upvalueindex(5))) return mlua_index_undefined(ls);
    return 0;
}

static int key_bits(uint16_t nkeys) {
    for (int i = 0; ; ++i) {
        if (nkeys <= ((uint32_t)1u << i)) return i;
    }
}

static void set_hashed_metatable(lua_State* ls, MLuaSymH const* fields, int cnt,
                          MLuaSymHash const* h, uint8_t const* g, bool strict) {
    if (cnt != h->nkeys) {
        luaL_error(ls, "key count mismatch: %d symbols, expected %d", cnt,
                   h->nkeys);
        return;
    }
    lua_createtable(ls, 0, 0);
    lua_pushlightuserdata(ls, (void*)fields);
    lua_pushlightuserdata(ls, (void*)h);
    lua_pushlightuserdata(ls, (void*)g);
    lua_pushinteger(ls, key_bits(h->nkeys));
    if (strict) lua_pushboolean(ls, strict);
    lua_pushcclosure(ls, &hashed_index, strict ? 5 : 4);
    lua_setfield(ls, -2, "__index");
    lua_setmetatable(ls, -2);
}

void mlua_new_module_hash_(lua_State* ls, MLuaSymH const* fields, int narr,
                           int nrec, MLuaSymHash const* h, uint8_t const* g) {
    lua_createtable(ls, narr, 0);
    set_hashed_metatable(ls, fields, nrec, h, g, true);
}

void mlua_new_class_hash_(
        lua_State* ls, char const* name, MLuaSymH const* fields, int cnt,
        MLuaSymHash const* h, uint8_t const* g, bool strict) {
    luaL_newmetatable(ls, name);
    set_hashed_metatable(ls, fields, cnt, h, g, strict);
    lua_pushvalue(ls, -1);
    lua_setfield(ls, -2, "__index");
}

extern MLuaModule const __start_mlua_module_registry[];
extern MLuaModule const __stop_mlua_module_registry[];

static int preload___index(lua_State* ls) {
    char const* name = luaL_checkstring(ls, 2);
    for (MLuaModule const* m = __start_mlua_module_registry;
            m != __stop_mlua_module_registry; ++m) {
        if (strcmp(m->name, name) == 0) {
            return lua_pushcfunction(ls, m->open), 1;
        }
    }
    return 0;
}

static int preload_next(lua_State* ls) {
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

static int preload___pairs(lua_State* ls) {
    lua_pushcfunction(ls, &preload_next);
    return 1;
}

static char const Preload_name[] = "mlua.Preload";

MLUA_SYMBOLS_NOHASH(Preload_syms) = {
    MLUA_SYM_V_NH(__index, function, &preload___index),
    MLUA_SYM_V_NH(__pairs, function, &preload___pairs),
};

static int return_ctx(lua_State* ls, int status, lua_KContext ctx) {
    return ctx;
}

static int Function___close(lua_State* ls) {
    // Call the function itself, passing through the remaining arguments. This
    // makes to-be-closed functions the equivalent of deferreds.
    lua_callk(ls, lua_gettop(ls) - 1, 0, 0, &return_ctx);
    return 0;
}

char const mlua_WeakK_name[] = "mlua.WeakK";

void mlua_register_modules(lua_State* ls) {
    // Require library "base".
    luaL_requiref(ls, "_G", luaopen_base, 1);
    lua_pop(ls, 1);  // _G

    // Create the Strict metatable, and set it on _G.
    lua_pushglobaltable(ls);
    luaL_newmetatable(ls, Strict_name);
    mlua_set_fields(ls, Strict_syms);
    lua_setmetatable(ls, -2);
    lua_pop(ls, 1);  // _G

    // Create the Module metatable.
    luaL_newmetatable(ls, Module_name);
    lua_pushglobaltable(ls);
    lua_setfield(ls, -2, "__index");
    lua_pop(ls, 1);  // Module

    // Create the Preload metatable, and set it on package.preload to load
    // compiled-in modules.
    luaL_requiref(ls, "package", luaopen_package, 0);
    lua_getfield(ls, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    luaL_newmetatable(ls, Preload_name);
    mlua_set_fields(ls, Preload_syms);
    lua_setmetatable(ls, -2);
    lua_pop(ls, 1);  // preload

    // Remove unused searchers.
    lua_getfield(ls, -1, "searchers");
    for (lua_Integer i = luaL_len(ls, -1); i > 1; --i) {
        lua_pushnil(ls);
        lua_seti(ls, -2, i);
    }
    lua_pop(ls, 2);  // searchers, package

    // Set globals.
    lua_pushstring(ls, LUA_RELEASE);
    lua_setglobal(ls, "_RELEASE");
    lua_pushinteger(ls, LUA_VERSION_NUM);
    lua_setglobal(ls, "_VERSION_NUM");
    lua_pushinteger(ls, LUA_VERSION_RELEASE_NUM);
    lua_setglobal(ls, "_RELEASE_NUM");
    lua_pushcfunction(ls, &global_module);
    lua_setglobal(ls, "module");
    lua_pushcfunction(ls, &global_try);
    lua_setglobal(ls, "try");

    // Set a metatable on functions.
    lua_pushcfunction(ls, &Function___close);  // Any function will do
    lua_createtable(ls, 0, 1);
    lua_pushcfunction(ls, &Function___close);
    lua_setfield(ls, -2, "__close");
    lua_setmetatable(ls, -2);
    lua_pop(ls, 1);

    // Create a metatable for weak keys.
    luaL_newmetatable(ls, mlua_WeakK_name);
    lua_pushliteral(ls, "__mode");
    lua_pushliteral(ls, "k");
    lua_rawset(ls, -3);
    lua_setglobal(ls, "WeakK");
}
