#include "mlua/main.h"

#include <stdio.h>

#ifdef LIB_PICO_STDIO
#include "pico/stdio.h"
#endif

#include "lua.h"
#include "lauxlib.h"

#include "mlua/lib.h"

// TODO: Run everything within a pcall

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

void mlua_main() {
    printf("=== Creating Lua state\n");
    lua_State* ls = luaL_newstate();
    mlua_open_libs(ls);

    printf("=== Loading main module\n");
    lua_getglobal(ls, "require");
    lua_pushliteral(ls, "main");
    lua_call(ls, 1, 1);

    if (try_getfield(ls, -1, "main") == LUA_OK) {
        printf("=== Running main function\n");
        lua_remove(ls, -2);  // Remove main module
        lua_call(ls, 0, 0);
    } else {
        printf("=== Main not found: %s\n", lua_tostring(ls, -1));
        lua_pop(ls, 2);
    }

    printf("=== Closing Lua state\n");
    lua_close(ls);
}

__attribute__((weak)) int main() {
#ifdef LIB_PICO_STDIO
    stdio_init_all();
#endif
    mlua_main();
    return 0;
}
