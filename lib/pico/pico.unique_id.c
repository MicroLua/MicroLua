// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "pico/unique_id.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

static int mod_get_unique_board_id(lua_State* ls) {
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    lua_pushlstring(ls, (char*)id.id, sizeof(id.id));
    return 1;
}

static int mod_get_unique_board_id_string(lua_State* ls) {
    char id[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    pico_get_unique_board_id_string(id, sizeof(id));
    lua_pushlstring(ls, id, sizeof(id) - 1);
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(BOARD_ID_SIZE, integer, PICO_UNIQUE_BOARD_ID_SIZE_BYTES),

    MLUA_SYM_F(get_unique_board_id, mod_),
    MLUA_SYM_F(get_unique_board_id_string, mod_),
};

MLUA_OPEN_MODULE(pico.unique_id) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
