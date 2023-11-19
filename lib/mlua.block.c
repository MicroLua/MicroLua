// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/block.h"

#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/util.h"

char const* mlua_block_dev_error(int err) {
    switch (err) {
    case MLUA_EINVAL: return "invalid argument";
    default: return "unknown error";
    }
}

char const mlua_BlockDev_name[] = "mlua.block.Dev";

MLuaBlockDev* mlua_new_BlockDev(lua_State* ls, size_t size, int nuv) {
    MLuaBlockDev* dev = lua_newuserdatauv(ls, size, nuv);
    luaL_getmetatable(ls, mlua_BlockDev_name);
    lua_setmetatable(ls, -2);
    return dev;
}

static int BlockDev_read(lua_State* ls) {
    MLuaBlockDev* dev = mlua_check_BlockDev(ls, 1);
    uint64_t off = mlua_check_int64(ls, 2);
    size_t size = luaL_checkinteger(ls, 3);
    luaL_Buffer buf;
    void* dst = luaL_buffinitsize(ls, &buf, size);
    int err = dev->read(dev, off, dst, size);
    if (err < 0) return mlua_push_fail(ls, mlua_block_dev_error(err));
    return luaL_pushresultsize(&buf, size), 1;
}

static int BlockDev_write(lua_State* ls) {
    MLuaBlockDev* dev = mlua_check_BlockDev(ls, 1);
    uint64_t off = mlua_check_int64(ls, 2);
    size_t len;
    void const* src = luaL_checklstring(ls, 3, &len);
    int err = dev->write(dev, off, src, len);
    if (err < 0) return mlua_push_fail(ls, mlua_block_dev_error(err));
    return lua_pushboolean(ls, true), 1;
}

static int BlockDev_erase(lua_State* ls) {
    MLuaBlockDev* dev = mlua_check_BlockDev(ls, 1);
    uint64_t off = mlua_check_int64(ls, 2);
    size_t size = luaL_checkinteger(ls, 3);
    int err = dev->erase(dev, off, size);
    if (err < 0) return mlua_push_fail(ls, mlua_block_dev_error(err));
    return lua_pushboolean(ls, true), 1;
}

static int BlockDev_sync(lua_State* ls) {
    MLuaBlockDev* dev = mlua_check_BlockDev(ls, 1);
    int err = dev->sync(dev);
    if (err < 0) return mlua_push_fail(ls, mlua_block_dev_error(err));
    return lua_pushboolean(ls, true), 1;
}

static int BlockDev_size(lua_State* ls) {
    MLuaBlockDev* dev = mlua_check_BlockDev(ls, 1);
    lua_pushinteger(ls, dev->size);
    lua_pushinteger(ls, dev->read_size);
    lua_pushinteger(ls, dev->write_size);
    lua_pushinteger(ls, dev->erase_size);
    return 4;
}

MLUA_SYMBOLS(BlockDev_syms) = {
    MLUA_SYM_F(read, BlockDev_),
    MLUA_SYM_F(write, BlockDev_),
    MLUA_SYM_F(erase, BlockDev_),
    MLUA_SYM_F(sync, BlockDev_),
    MLUA_SYM_F(size, BlockDev_),
};

MLUA_SYMBOLS(module_syms) = {
};

MLUA_OPEN_MODULE(mlua.block) {
    mlua_require(ls, "mlua.int64", false);

    // Create the BlockDev class.
    mlua_new_class(ls, mlua_BlockDev_name, BlockDev_syms, true);
    lua_pop(ls, 1);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
