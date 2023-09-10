#include "mlua/module.h"

#cmakedefine DATA 1
#cmakedefine SYMBOLS 1

#ifdef DATA

#include "lauxlib.h"

static char const data[] = {
#include "@DATA@"
};

MLUA_OPEN_MODULE(@MOD@) {
    if (luaL_loadbufferx(ls, data, sizeof(data)-1, "@MOD@", "bt") != LUA_OK) {
        return luaL_error(ls, "failed to load '@MOD@':\n\t%s",
                          lua_tostring(ls, -1));
    }
    lua_pushstring(ls, "@MOD@");
    lua_call(ls, 1, 1);
    return 1;
}

#elif SYMBOLS

@INCLUDE@
#include "mlua/util.h"

MLUA_SYMBOLS(module_syms) = {
#include "@SYMBOLS@"
};

MLUA_OPEN_MODULE(@MOD@) {
    mlua_new_table(ls, 0, module_syms);  // Intentionally non-strict
    return 1;
}

#else

MLUA_REGISTER_MODULE(@MOD@, luaopen_@SYM@);

#endif
