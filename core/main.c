#include "mlua/main.h"

#include <stdio.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef LIB_PICO_STDIO
#include "pico/stdio.h"
#endif

#include "mlua/lib.h"

static int getfield(lua_State* ls) {
    lua_gettable(ls, -2);
    return 1;
}

static int try_getfield(lua_State* ls, int index, char const* k) {
    index = lua_absindex(ls, index);
    lua_pushcfunction(ls, getfield);
    lua_pushvalue(ls, index);
    lua_pushstring(ls, k);
    return lua_pcall(ls, 2, 1, 0);
}

static int pmain(lua_State* ls) {
    // Set up module loading.
    mlua_open_libs(ls);

    // Require the "main" module.
    lua_getglobal(ls, "require");
    lua_pushliteral(ls, "main");
    lua_call(ls, 1, 1);

    // If the "main" module has a "main" function, call it.
    if (try_getfield(ls, -1, "main") == LUA_OK) {
        lua_remove(ls, -2);  // Remove main module
        lua_call(ls, 0, 0);
    } else {
        lua_pop(ls, 2);  // Remove error and main module
    }
    return 0;
}

void mlua_main() {
    lua_State* ls = luaL_newstate();
    lua_pushcfunction(ls, pmain);
    int err = lua_pcall(ls, 0, 0, 0);
    if (err != LUA_OK) {
        printf("ERROR: %s\n", lua_tostring(ls, -1));
        lua_pop(ls, 1);
    }
    lua_close(ls);
}

__attribute__((weak)) int main() {
#ifdef LIB_PICO_STDIO
    stdio_init_all();
#endif
    mlua_main();
    return 0;
}
