// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "pico/rand.h"

#include "lauxlib.h"
#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/util.h"

int mod_get_rand_128(lua_State* ls) {
    rng_128_t value;
    get_rand_128(&value);
    mlua_push_int64(ls, value.r[0]);
    mlua_push_int64(ls, value.r[1]);
    return 2;
}

MLUA_FUNC_R0(mod_,, get_rand_32, lua_pushinteger)
MLUA_FUNC_R0(mod_,, get_rand_64, mlua_push_int64)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(get_rand_32, mod_),
    MLUA_SYM_F(get_rand_64, mod_),
    MLUA_SYM_F(get_rand_128, mod_),
};

MLUA_OPEN_MODULE(pico.rand) {
    mlua_require(ls, "mlua.int64", false);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
