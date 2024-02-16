// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_MLUA_ERRORS_H
#define _MLUA_LIB_COMMON_MLUA_ERRORS_H

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// The list of error names, values and descriptions. Do not change the value of
// existing errors.
#define MLUA_ERRORS \
    MLUA_ERR(EBADF, 1, "bad file descriptor") \
    MLUA_ERR(EBUSY, 16, "device or resource busy") \
    MLUA_ERR(ECORRUPT, 2, "corrupted") \
    MLUA_ERR(EEXIST, 3, "file exists") \
    MLUA_ERR(EFBIG, 4, "file too large") \
    MLUA_ERR(EINVAL, 5, "invalid argument") \
    MLUA_ERR(EIO, 6, "input / output error") \
    MLUA_ERR(EISDIR, 7, "is a directory") \
    MLUA_ERR(ENAMETOOLONG, 8, "filename too long") \
    MLUA_ERR(ENODATA, 9, "no data / attribute available") \
    MLUA_ERR(ENOENT, 10, "no such file or directory") \
    MLUA_ERR(ENOMEM, 11, "no memory available") \
    MLUA_ERR(ENOSPC, 12, "no space left on device") \
    MLUA_ERR(ENOTCONN, 17, "transport endpoint is not connected") \
    MLUA_ERR(ENOTDIR, 13, "not a directory") \
    MLUA_ERR(ENOTEMPTY, 14, "directory not empty") \
    MLUA_ERR(EROFS, 15, "read-only filesystem") \

// Error codes.
typedef enum MLuaErrno {
    MLUA_EOK = 0,
    MLUA_ERR_MASK = -0x00010000,    // 0xffff0000
    MLUA_ERR_MARKER = -0x14530000,  // 0xebad0000

#define MLUA_ERR(name, value, msg) MLUA_ ## name = MLUA_ERR_MARKER + value,
MLUA_ERRORS
#undef MLUA_ERR
} MLuaErrno;

// Return a string describing an error code.
char const* mlua_err_msg(int err);

// Push a fail, an error message and an error code, and return the number of
// pushed values.
int mlua_err_push(lua_State* ls, int err);

#ifdef __cplusplus
}
#endif

#endif
