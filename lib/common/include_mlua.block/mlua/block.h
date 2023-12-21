// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_MLUA_BLOCK_H
#define _MLUA_LIB_COMMON_MLUA_BLOCK_H

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

    // The size of the block device.
    uint64_t size;

    // The minimum size of a read. All reads must be multiples of this value.
    uint32_t read_size;

    // The minimum size of a write. All writes must be multiples of this value.
    uint32_t write_size;

    // The minimum size of an erase. All erases must be multiples of this value.
    uint32_t erase_size;
};

// Push a BlockDev value to the stack.
void* mlua_block_push(lua_State* ls, size_t size, int nuv);

// Get a BlockDev value, or raise an error if the argument is not a BlockDev
// userdata.
MLuaBlockDev* mlua_block_check(lua_State* ls, int arg);

#ifdef __cplusplus
}
#endif

#endif
