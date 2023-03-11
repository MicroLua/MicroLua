#include "mlua/lib.h"

#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

static mlua_lib* libs = NULL;

void mlua_register_lib(mlua_lib* lib) {
    lib->next = libs;
    libs = lib;
}

void mlua_open_libs(lua_State* ls) {
    // Require library "base".
    luaL_requiref(ls, "_G", luaopen_base, 1);
    lua_pop(ls, 1);

    // Require library "package".
    luaL_requiref(ls, "package", luaopen_package, 0);
    lua_getfield(ls, -1, "preload");
    lua_remove(ls, -2);  // Remove package

    mlua_lib* lib = libs;
    while (lib != NULL) {
        lua_pushcfunction(ls, lib->open);
        lua_setfield(ls, -2, lib->name);
        lib = lib->next;
    }

    lua_pop(ls, 1);  // Remove preload
}
