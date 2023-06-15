#include "pico/unique_id.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/util.h"

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

static MLuaReg const module_regs[] = {
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(get_unique_board_id),
    MLUA_SYM(get_unique_board_id_string),
#undef MLUA_SYM
    {NULL},
};

int luaopen_pico_unique_id(lua_State* ls) {
    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
