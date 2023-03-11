#include <stdio.h>

#ifdef LIB_PICO_STDIO
#include "pico/stdio.h"
#endif

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "mlua/main.h"
#include "mlua/file.h"

int mlua_run_file(lua_State* ls, char const* path) {
    size_t size;
    char const* code = mlua_get_file(path, &size);
    if (code == NULL) {
        lua_pushfstring(ls, "script not found: %s", path);
        return LUA_ERRFILE;
    }
    printf("== File size: %d\n", size);
    int err = luaL_loadbufferx(ls, code, size, path, "bt");
    if (err != LUA_OK) return err;
    err = luaL_loadstring(ls, code);
    if (err != LUA_OK) return err;
    return lua_pcall(ls, 0, 0, 0);
}

void mlua_main() {
    printf("=== Creating Lua state\n");
    lua_State* ls = luaL_newstate();
    luaL_openlibs(ls);

    printf("=== Executing script\n");
    if (mlua_run_file(ls, "main.lua") != LUA_OK) {
        printf("=== Failed to run main.lua: %s\n", lua_tostring(ls, -1));
        lua_pop(ls, 1);
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
