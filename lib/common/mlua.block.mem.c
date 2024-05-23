// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "mlua/block.h"
#include "mlua/errors.h"
#include "mlua/module.h"
#include "mlua/util.h"

typedef struct Dev {
    MLuaBlockDev dev;
    void* start;
} Dev;

static int mem_dev_read(MLuaBlockDev* dev, uint64_t off, void* dst,
                        size_t size) {
    Dev* d = (Dev*)dev;
    if (off + size > d->dev.size) return MLUA_EINVAL;
    memcpy(dst, d->start + off, size);
    return MLUA_EOK;
}

static int mem_dev_write(MLuaBlockDev* dev, uint64_t off, void const* src,
                         size_t size) {
    Dev* d = (Dev*)dev;
    if (off + size > d->dev.size) return MLUA_EINVAL;
    memcpy(d->start + off, src, size);
    return MLUA_EOK;
}

static int mem_dev_erase(MLuaBlockDev* dev, uint64_t off, size_t size) {
    Dev* d = (Dev*)dev;
    if (off + size > d->dev.size) return MLUA_EINVAL;
    memset(d->start + off, 0xff, size);
    return MLUA_EOK;
}

static int mem_dev_sync(MLuaBlockDev* dev) { return MLUA_EOK; }

static int mod_new(lua_State* ls) {
    MLuaBuffer buf;
    luaL_argexpected(ls, mlua_get_buffer(ls, 1, &buf) && buf.vt == NULL, 1,
                     "memory buffer");
    size_t write_size = luaL_optinteger(ls, 2, 256);
    size_t erase_size = luaL_optinteger(ls, 3, 256);

    Dev* dev = mlua_block_push(ls, sizeof(Dev), 1);
    lua_pushvalue(ls, 1);
    lua_setiuservalue(ls, -2, 1);  // Keep buffer alive
    dev->dev.read = &mem_dev_read,
    dev->dev.write = &mem_dev_write,
    dev->dev.erase = &mem_dev_erase,
    dev->dev.sync = &mem_dev_sync,
    dev->dev.size = buf.size - (buf.size % erase_size);
    dev->dev.read_size = 1;
    dev->dev.write_size = write_size;
    dev->dev.erase_size = erase_size;
    dev->start = buf.ptr;
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(new, mod_),
};

MLUA_OPEN_MODULE(mlua.block.mem) {
    mlua_require(ls, "mlua.block", false);
    mlua_require(ls, "mlua.mem", false);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
