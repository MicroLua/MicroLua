#include "mlua/lib.h"

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

static mlua_lib const module
    __attribute__((__section__("mlua_modules"), __used__))
    = {.name = "@MOD@", .open = luaopen_@SYM@};
