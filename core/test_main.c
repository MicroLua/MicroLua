#include <stdio.h>

#include "pico/stdlib.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

int main() {
    stdio_init_all();

    printf("=== Creating Lua state\n");
    lua_State* ls = luaL_newstate();
    luaL_openlibs(ls);

    printf("=== Executing script\n");
    char const* script =
        "print(string.format('maxinteger: %d', math.maxinteger))\n"
        "print(string.format('huge: %f', math.huge))\n"
        // "local i = 0\n"
        // "while true do\n"
        // "  print(string.format('Hello, MicroLua! (%d)', i))\n"
        // "  i = i + 1\n"
        // "end\n"
    ;
    if (luaL_loadstring(ls, script) != LUA_OK) {
        printf("=== loadstring: Failed to load script: %s\n", lua_tostring(ls, -1));
        lua_pop(ls, 1);
        return 1;
    }
    if (lua_pcall(ls, 0, 0, 0) != LUA_OK) {
        printf("=== pcall: Script failed: %s\n", lua_tostring(ls, -1));
        lua_pop(ls, 1);
        return 1;
    }

    printf("=== Closing Lua state\n");
    lua_close(ls);

    printf("=== Exiting\n");
    return 0;
}
