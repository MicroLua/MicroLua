// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "mlua/module.h"

// We need at least one module, otherwise the symbols below aren't defined and
// linking fails.
MLUA_OPEN_MODULE(mlua) {
    lua_createtable(ls, 0, 0);
    return 1;
}

extern MLuaModule const __start_mlua_module_registry[];
extern MLuaModule const __stop_mlua_module_registry[];

// Set up all compiled-in modules in package.preload.
static void preload_modules(lua_State* ls) {
    luaL_requiref(ls, "package", luaopen_package, 0);
    lua_getfield(ls, -1, "preload");
    lua_remove(ls, -2);
    for (MLuaModule const* m = __start_mlua_module_registry;
            m != __stop_mlua_module_registry; ++m) {
        lua_pushcfunction(ls, m->open);
        lua_setfield(ls, -2, m->name);
    }
}

// Include the script text.
__asm__(
    ".section \".rodata\"\n"
    ".global script, script_end\n"
    "script:     .incbin \"@SCRIPT@\"\n"
    "script_end: .byte 0\n"
    ".previous\n"
);

extern char const script[];
extern char const script_end[];

int pmain(lua_State* ls) {
    int argc = lua_tointeger(ls, 1);
    char** argv = lua_touserdata(ls, 2);
    luaL_openlibs(ls);
    preload_modules(ls);
    if (luaL_loadbufferx(ls, script, script_end - script,
                         "@@FILE@", "t") != LUA_OK) {
        return lua_error(ls);
    }
    for (int i = 1; i < argc; ++i) lua_pushstring(ls, argv[i]);
    if (lua_pcall(ls, argc - 1, 1, 0) != LUA_OK) return lua_error(ls);
    return 1;
}

int main(int argc, char* argv[]) {
    lua_State* ls = luaL_newstate();
    if (ls == NULL) {
        fprintf(stderr, "ERROR: failed to create Lua state\n");
        return EXIT_FAILURE;
    }
    lua_pushcfunction(ls, &pmain);
    lua_pushinteger(ls, argc);
    lua_pushlightuserdata(ls, argv);
    int res = EXIT_FAILURE;
    if (lua_pcall(ls, 2, 1, 0) != LUA_OK) {
        fprintf(stderr, "ERROR: %s\n", lua_tostring(ls, -1));
    } else if (lua_isnil(ls, -1)) {
        res = EXIT_SUCCESS;
    } else if (lua_isinteger(ls, -1)) {
        res = lua_tointeger(ls, -1);
    } else if (lua_isstring(ls, -1)) {
        fprintf(stderr, "ERROR: %s\n", lua_tostring(ls, -1));
    }
    lua_pop(ls, 1);
    lua_close(ls);
    return res;
}
