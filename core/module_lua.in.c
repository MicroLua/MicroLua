// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/module.h"

#include "lua.h"
#include "lauxlib.h"

#if @INCBIN@

__asm__(
    ".section \".rodata\"\n"
    ".local data, data_end\n"
    "data:\n"
    ".incbin \"@SRC@\"\n"
    "data_end:\n"
    ".previous\n"
);

extern char const data[];
extern char const data_end[];
#define data_size (data_end - data)

#else

static char const data[] = {@DATA@};
#define data_size (sizeof(data))

#endif

MLUA_OPEN_MODULE(@MOD@) {
    if (luaL_loadbufferx(ls, data, data_size, "@MOD@", "bt") != LUA_OK) {
        return luaL_error(ls, "failed to load '@MOD@':\n\t%s",
                          lua_tostring(ls, -1));
    }
    lua_pushliteral(ls, "@MOD@");
    lua_call(ls, 1, 1);
    return 1;
}
