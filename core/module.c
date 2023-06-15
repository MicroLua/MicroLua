#include "mlua/module.h"

#include <string.h>

#include "lauxlib.h"
#include "lualib.h"

void mlua_register_modules(lua_State* ls) {
    // Require library "base".
    luaL_requiref(ls, "_G", luaopen_base, 1);
    lua_pop(ls, 1);

    // Require library "package".
    luaL_requiref(ls, "package", luaopen_package, 0);
    lua_getfield(ls, -1, "preload");
    lua_remove(ls, -2);  // Remove package

    // Register compiled-in modules with package.preload.
    extern MLuaModule const __start_mlua_module_registry;
    extern MLuaModule const __stop_mlua_module_registry;
    for (MLuaModule const* m = &__start_mlua_module_registry;
            m < &__stop_mlua_module_registry; ++m) {
        lua_pushcfunction(ls, m->open);
        lua_setfield(ls, -2, m->name);
    }
    lua_pop(ls, 1);  // Remove preload
}
