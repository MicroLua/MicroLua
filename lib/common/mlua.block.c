// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/block.h"

#include "mlua/errors.h"
#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/util.h"

static char const Dev_name[] = "mlua.block.Dev";

void* mlua_block_push(lua_State* ls, size_t size, int nuv) {
    MLuaBlockDev* dev = lua_newuserdatauv(ls, size, nuv);
    luaL_getmetatable(ls, Dev_name);
    lua_setmetatable(ls, -2);
    return dev;
}

MLuaBlockDev* mlua_block_check(lua_State* ls, int arg) {
    void* ptr = luaL_checkudata(ls, arg, Dev_name);
    if (ptr == NULL || lua_rawlen(ls, arg) != sizeof(ptr)) return ptr;
    return *((MLuaBlockDev**)ptr);
}

static int Dev_read(lua_State* ls) {
    MLuaBlockDev* dev = mlua_block_check(ls, 1);
    uint64_t off = mlua_check_int64(ls, 2);
    size_t size = luaL_checkinteger(ls, 3);
    luaL_Buffer buf;
    void* dst = luaL_buffinitsize(ls, &buf, size);
    int err = dev->read(dev, off, dst, size);
    if (err < 0) return mlua_err_push(ls, err);
    return luaL_pushresultsize(&buf, size), 1;
}

static int Dev_write(lua_State* ls) {
    MLuaBlockDev* dev = mlua_block_check(ls, 1);
    uint64_t off = mlua_check_int64(ls, 2);
    size_t len;
    void const* src = luaL_checklstring(ls, 3, &len);
    int err = dev->write(dev, off, src, len);
    if (err < 0) return mlua_err_push(ls, err);
    return lua_pushboolean(ls, true), 1;
}

static int Dev_erase(lua_State* ls) {
    MLuaBlockDev* dev = mlua_block_check(ls, 1);
    uint64_t off = mlua_check_int64(ls, 2);
    size_t size = luaL_checkinteger(ls, 3);
    int err = dev->erase(dev, off, size);
    if (err < 0) return mlua_err_push(ls, err);
    return lua_pushboolean(ls, true), 1;
}

static int Dev_sync(lua_State* ls) {
    MLuaBlockDev* dev = mlua_block_check(ls, 1);
    int err = dev->sync(dev);
    if (err < 0) return mlua_err_push(ls, err);
    return lua_pushboolean(ls, true), 1;
}

static int Dev_size(lua_State* ls) {
    MLuaBlockDev* dev = mlua_block_check(ls, 1);
    lua_pushinteger(ls, dev->size);
    lua_pushinteger(ls, dev->read_size);
    lua_pushinteger(ls, dev->write_size);
    lua_pushinteger(ls, dev->erase_size);
    return 4;
}

MLUA_SYMBOLS(Dev_syms) = {
    MLUA_SYM_F(read, Dev_),
    MLUA_SYM_F(write, Dev_),
    MLUA_SYM_F(erase, Dev_),
    MLUA_SYM_F(sync, Dev_),
    MLUA_SYM_F(size, Dev_),
};

MLUA_SYMBOLS(module_syms) = {
};

MLUA_OPEN_MODULE(mlua.block) {
    mlua_require(ls, "mlua.int64", false);

    // Create the Dev class.
    mlua_new_class(ls, Dev_name, Dev_syms, mlua_nosyms);
    lua_pop(ls, 1);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
