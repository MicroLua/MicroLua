#include "mlua/lib.h"

#include "lua.h"

#cmakedefine DATA 1
#ifdef DATA

#include "lauxlib.h"

static char const data[] = {
#include "@DATA@"
};

extern int luaopen_@SYM@(lua_State* ls) {
    int err = luaL_loadbufferx(ls, data, sizeof(data)-1, "@LIB@", "bt");
    if (err != LUA_OK) {
        return luaL_error(ls, "failed to load '@LIB@':\n\t%s",
                          lua_tostring(ls, -1));
    }
    lua_pushstring(ls, "@LIB@");
    lua_call(ls, 1, 1);
    return 1;
}

#else

extern int luaopen_@SYM@(lua_State* ls);

#endif

static mlua_lib lib = {.name = "@LIB@", .open = luaopen_@SYM@};

static __attribute__((constructor)) void register_lib(void) {
    mlua_register_lib(&lib);
}
