// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/fs.h"

#include "mlua/module.h"
#include "mlua/util.h"

static int mod_join(lua_State* ls) {
    int top = lua_gettop(ls);
    if (top == 0) return lua_pushliteral(ls, ""), 1;
    size_t len;
    luaL_Buffer buf;
    for (int i = 1; i <= top; ++i) {
        char const* el = luaL_checklstring(ls, i, &len);
        if (i == 1 || el[0] == '/') {
            lua_settop(ls, top);
            luaL_buffinitsize(ls, &buf, len);
        } else {
            size_t bl = luaL_bufflen(&buf);
            if (bl > 0 && luaL_buffaddr(&buf)[bl - 1] != '/') {
                luaL_addchar(&buf, '/');
            }
        }
        luaL_addlstring(&buf, el, len);
    }
    luaL_pushresult(&buf);
    return 1;
}

static int mod_split(lua_State* ls) {
    size_t len;
    char const* path = luaL_checklstring(ls, 1, &len);
    char const* b = path + len - 1;
    while (b != path - 1 && *b != '/') --b;
    ++b;
    char const* e = b;
    while (e != path && e[-1] == '/') --e;
    if (e == path) e = b;
    lua_pushlstring(ls, path, e - path);
    lua_pushlstring(ls, b, path + len - b);
    return 2;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(TYPE_REG, integer, MLUA_FS_TYPE_REG),
    MLUA_SYM_V(TYPE_DIR, integer, MLUA_FS_TYPE_DIR),
    MLUA_SYM_V(O_RDONLY, integer, MLUA_FS_O_RDONLY),
    MLUA_SYM_V(O_WRONLY, integer, MLUA_FS_O_WRONLY),
    MLUA_SYM_V(O_RDWR, integer, MLUA_FS_O_RDWR),
    MLUA_SYM_V(O_CREAT, integer, MLUA_FS_O_CREAT),
    MLUA_SYM_V(O_EXCL, integer, MLUA_FS_O_EXCL),
    MLUA_SYM_V(O_TRUNC, integer, MLUA_FS_O_TRUNC),
    MLUA_SYM_V(O_APPEND, integer, MLUA_FS_O_APPEND),
    MLUA_SYM_V(SEEK_SET, integer, MLUA_FS_SEEK_SET),
    MLUA_SYM_V(SEEK_CUR, integer, MLUA_FS_SEEK_CUR),
    MLUA_SYM_V(SEEK_END, integer, MLUA_FS_SEEK_END),

    MLUA_SYM_F(join, mod_),
    MLUA_SYM_F(split, mod_),
};

MLUA_OPEN_MODULE(mlua.fs) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
