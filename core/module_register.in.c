#include "mlua/module.h"

#include "lua.h"

#cmakedefine DATA 1
#ifdef DATA

#include "lauxlib.h"

static char const data[] = {
#include "@DATA@"
};

extern int luaopen_@SYM@(lua_State* ls) {
    int err = luaL_loadbufferx(ls, data, sizeof(data)-1, "@MOD@", "bt");
    if (err != LUA_OK) {
        return luaL_error(ls, "failed to load '@MOD@':\n\t%s",
                          lua_tostring(ls, -1));
    }
    lua_pushstring(ls, "@MOD@");
    lua_call(ls, 1, 1);
    return 1;
}

#else

extern int luaopen_@SYM@(lua_State* ls);

#endif

static MLuaModule const module
    __attribute__((__section__("mlua_module_registry"), __used__))
    = {.name = "@MOD@", .open = luaopen_@SYM@};
