// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/errors.h"

#include <assert.h>

#include "mlua/module.h"
#include "mlua/util.h"

#define MLUA_ERR(name, value, msg) \
    static_assert(MLUA_ ## name < 0, "MLUA_" #name " not negative"); \
    static_assert(MLUA_ ## name > MLUA_ERR_MARKER, \
                  "MLUA_" #name " is smaller than the marker"); \
    static_assert((MLUA_ ## name & MLUA_ERR_MASK) == MLUA_ERR_MARKER, \
                  "MLUA_" #name " has no marker");
MLUA_ERRORS
#undef MLUA_ERR

char const* mlua_err_msg(int err) {
    switch (err) {
    case MLUA_EOK: return "no error";
#define MLUA_ERR(name, value, msg) case MLUA_ ## name: return msg;
MLUA_ERRORS
#undef MLUA_ERR
    default: return "unknown error";
    }
}

int mlua_err_push(lua_State* ls, int err) {
    mlua_push_fail(ls, mlua_err_msg(err));
    lua_pushinteger(ls, err);
    return 3;
}

static int mod_message(lua_State* ls) {
    return lua_pushstring(ls, mlua_err_msg(luaL_checkinteger(ls, 1))), 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(EOK, integer, MLUA_EOK),
    // We can't use MLUA_ERRORS here due to read-only table parsing.
    MLUA_SYM_V(EBADF, integer, MLUA_EBADF),
    MLUA_SYM_V(EBUSY, integer, MLUA_EBUSY),
    MLUA_SYM_V(ECORRUPT, integer, MLUA_ECORRUPT),
    MLUA_SYM_V(EEXIST, integer, MLUA_EEXIST),
    MLUA_SYM_V(EFBIG, integer, MLUA_EFBIG),
    MLUA_SYM_V(EINVAL, integer, MLUA_EINVAL),
    MLUA_SYM_V(EIO, integer, MLUA_EIO),
    MLUA_SYM_V(EISDIR, integer, MLUA_EISDIR),
    MLUA_SYM_V(ENAMETOOLONG, integer, MLUA_ENAMETOOLONG),
    MLUA_SYM_V(ENODATA, integer, MLUA_ENODATA),
    MLUA_SYM_V(ENOENT, integer, MLUA_ENOENT),
    MLUA_SYM_V(ENOMEM, integer, MLUA_ENOMEM),
    MLUA_SYM_V(ENOSPC, integer, MLUA_ENOSPC),
    MLUA_SYM_V(ENOTCONN, integer, MLUA_ENOTCONN),
    MLUA_SYM_V(ENOTDIR, integer, MLUA_ENOTDIR),
    MLUA_SYM_V(ENOTEMPTY, integer, MLUA_ENOTEMPTY),
    MLUA_SYM_V(EROFS, integer, MLUA_EROFS),

    MLUA_SYM_F(message, mod_),
};

MLUA_OPEN_MODULE(mlua.errors) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
