#include "mlua/module.h"

#cmakedefine DATA 1
#cmakedefine CONFIG 1

#ifdef DATA

#include "lauxlib.h"

static char const data[] = {
#include "@DATA@"
};

int luaopen_@SYM@(lua_State* ls) {
    if (luaL_loadbufferx(ls, data, sizeof(data)-1, "@MOD@", "bt") != LUA_OK) {
        return luaL_error(ls, "failed to load '@MOD@':\n\t%s",
                          lua_tostring(ls, -1));
    }
    lua_pushstring(ls, "@MOD@");
    lua_call(ls, 1, 1);
    return 1;
}

#elif CONFIG

@INCLUDE@
#include "mlua/util.h"

static MLuaSym const module_syms[] = {
#include "@CONFIG@"
};

int luaopen_@SYM@(lua_State* ls) {
    mlua_new_table(ls, 0, module_syms);  // Intentionally non-strict
    return 1;
}

#else

extern int luaopen_@SYM@(lua_State* ls);

#endif

static MLuaModule const module
    __attribute__((__section__("mlua_module_registry"), __used__))
    = {.name = "@MOD@", .open = luaopen_@SYM@};
