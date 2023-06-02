#include "mlua/lib.h"

#include <string.h>

#include "lauxlib.h"
#include "lualib.h"

// Silence link-time warnings.
__attribute__((weak)) int _link(char const* old, char const* new) { return -1; }
__attribute__((weak)) int _unlink(char const* file) { return -1; }

void mlua_open_libs(lua_State* ls) {
    // Require library "base".
    luaL_requiref(ls, "_G", luaopen_base, 1);
    lua_pop(ls, 1);

    // Require library "package".
    luaL_requiref(ls, "package", luaopen_package, 0);
    lua_getfield(ls, -1, "preload");
    lua_remove(ls, -2);  // Remove package

    // Register compiled-in modules with package.preload.
    extern struct mlua_lib const __start_mlua_modules;
    extern struct mlua_lib const __stop_mlua_modules;
    for (mlua_lib const* m = &__start_mlua_modules;
            m < &__stop_mlua_modules; ++m) {
        lua_pushcfunction(ls, m->open);
        lua_setfield(ls, -2, m->name);
    }
    lua_pop(ls, 1);  // Remove preload
}
