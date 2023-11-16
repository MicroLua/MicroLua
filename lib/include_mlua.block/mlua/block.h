// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_MLUA_BLOCK_H
#define _MLUA_LIB_MLUA_BLOCK_H

#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// A block device.
typedef struct MLuaBlockDev MLuaBlockDev;
struct MLuaBlockDev {
    // Read from the block device. "off" and "size" must be multiples of
    // "read_size".
    int (*read)(MLuaBlockDev* dev, uint64_t off, void* dst, size_t size);

    // Write to the block device. "off" and "size" must be multiples of
    // "write_size".
    int (*write)(MLuaBlockDev* dev, uint64_t off, void const* src, size_t size);

    // Erase a range of the block device. "off" and "size" must be multiples of
    // "erase_size".
    int (*erase)(MLuaBlockDev* dev, uint64_t off, size_t size);

    // Flush all writes to the block device.
    int (*sync)(MLuaBlockDev* dev);

    // Return the message corresponding to an error code.
    char const* (*error)(int err);

    // The size of the block device.
    uint64_t size;

    // The minimum size of a read. All reads must be multiples of this value.
    uint32_t read_size;

    // The minimum size of a write. All writes must be multiples of this value.
    uint32_t write_size;

    // The minimum size of an erase. All erases must be multiples of this value.
    uint32_t erase_size;
};

// Create a new BlockDev value on the stack.
MLuaBlockDev* mlua_new_BlockDev(lua_State* ls, size_t size, int nuv);

// Get a BlockDev value, or raise an error if the argument is not a BlockDev
// userdata.
static inline MLuaBlockDev* mlua_check_BlockDev(lua_State* ls, int arg) {
    extern char const mlua_BlockDev_name[];
    return luaL_checkudata(ls, arg, mlua_BlockDev_name);
}

#ifdef __cplusplus
}
#endif

#endif
