// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_MLUA_ERRORS_H
#define _MLUA_LIB_MLUA_ERRORS_H

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MLUA_ERRORS \
    MLUA_ERR(EBADF, "bad file descriptor") \
    MLUA_ERR(ECORRUPT, "corrupted") \
    MLUA_ERR(EEXIST, "file exists") \
    MLUA_ERR(EFBIG, "file too large") \
    MLUA_ERR(EINVAL, "invalid argument") \
    MLUA_ERR(EIO, "input / output error") \
    MLUA_ERR(EISDIR, "is a directory") \
    MLUA_ERR(ENAMETOOLONG, "filename too long") \
    MLUA_ERR(ENOATTR, "no data / attr available") \
    MLUA_ERR(ENOENT, "no such file or directory") \
    MLUA_ERR(ENOMEM, "no memory available") \
    MLUA_ERR(ENOSPC, "no space left on device") \
    MLUA_ERR(ENOTDIR, "not a directory") \
    MLUA_ERR(ENOTEMPTY, "directory not empty") \

// Error codes.
enum MLuaErrno {
    MLUA_EOK = 0,
    MLUA_ERR_MASK = -0x00010000,    // 0xffff0000
    MLUA_ERR_MARKER = -0x14530000,  // 0xebad0000

#define MLUA_ERR(code, msg) MLUA_ ## code,
MLUA_ERRORS
#undef MLUA_ERR
};

// Return a string describing an error code.
char const* mlua_err_msg(int err);

// Push a fail, an error message and an error code, and return the number of
// pushed values.
int mlua_err_push(lua_State* ls, int err);

#ifdef __cplusplus
}
#endif

#endif
