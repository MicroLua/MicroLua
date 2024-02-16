// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_MLUA_FS_H
#define _MLUA_LIB_COMMON_MLUA_FS_H

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// File types.
typedef enum MLuaFileType {
    MLUA_FS_TYPE_UNKNOWN = 0,
    MLUA_FS_TYPE_REG = 1,
    MLUA_FS_TYPE_DIR = 2,
} MLuaFileType;

// Flags that can be provided to open().
typedef enum MLuaOpenFlag {
    MLUA_FS_O_RDONLY    = 0x0001,
    MLUA_FS_O_WRONLY    = 0x0002,
    MLUA_FS_O_RDWR      = 0x0003,
    MLUA_FS_O_CREAT     = 0x0100,
    MLUA_FS_O_EXCL      = 0x0200,
    MLUA_FS_O_TRUNC     = 0x0400,
    MLUA_FS_O_APPEND    = 0x0800,
} MLuaOpenFlag;

// Values that can be passed to seek() as "whence".
typedef enum MLuaSeek {
    MLUA_FS_SEEK_SET = 0,
    MLUA_FS_SEEK_CUR = 1,
    MLUA_FS_SEEK_END = 2,
} MLuaSeek;

#ifdef __cplusplus
}
#endif

#endif
