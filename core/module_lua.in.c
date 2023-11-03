// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/module.h"

#include "lua.h"
#include "lauxlib.h"

static char const data[] = {
@DATA@
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
