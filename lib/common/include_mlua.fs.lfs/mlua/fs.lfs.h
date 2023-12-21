// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_MLUA_FS_LFS_H
#define _MLUA_LIB_COMMON_MLUA_FS_LFS_H

#include <stdbool.h>

#include "mlua/block.h"

#ifdef __cplusplus
extern "C" {
#endif

// Create a new global LFS filesystem on the given block device.
void* mlua_fs_lfs_alloc(MLuaBlockDev* dev);

// Mount a global LFS filesystem.
int mlua_fs_lfs_mount(void* fs);

// Push a Filesystem value to the stack.
void mlua_fs_lfs_push(lua_State* ls, void* fs);

#ifdef __cplusplus
}
#endif

#endif
