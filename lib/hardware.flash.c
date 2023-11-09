// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "hardware/flash.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

int mod_range_program(lua_State* ls) {
    uint32_t offs = luaL_checkinteger(ls, 1);
    size_t len;
    uint8_t const* data = (uint8_t const*)luaL_checklstring(ls, 2, &len);
    flash_range_program(offs, data, len);
    return 0;
}

int mod_get_unique_id(lua_State* ls) {
    luaL_Buffer buf;
    uint8_t* dest = (uint8_t*)luaL_buffinitsize(ls, &buf,
                                                FLASH_UNIQUE_ID_SIZE_BYTES);
    flash_get_unique_id(dest);
    luaL_pushresultsize(&buf, FLASH_UNIQUE_ID_SIZE_BYTES);
    return 1;
}

int mod_do_cmd(lua_State* ls) {
    size_t len;
    uint8_t const* txbuf = (uint8_t const*)luaL_checklstring(ls, 1, &len);
    luaL_Buffer buf;
    uint8_t* dest = (uint8_t*)luaL_buffinitsize(ls, &buf, len);
    flash_do_cmd(txbuf, dest, len);
    luaL_pushresultsize(&buf, len);
    return 1;
}

MLUA_FUNC_V2(mod_, flash_, range_erase, luaL_checkinteger, luaL_checkinteger)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(PAGE_SIZE, integer, FLASH_PAGE_SIZE),
    MLUA_SYM_V(SECTOR_SIZE, integer, FLASH_SECTOR_SIZE),
    MLUA_SYM_V(BLOCK_SIZE, integer, FLASH_BLOCK_SIZE),
    MLUA_SYM_V(UNIQUE_ID_SIZE_BYTES, integer, FLASH_UNIQUE_ID_SIZE_BYTES),

    MLUA_SYM_F(range_erase, mod_),
    MLUA_SYM_F(range_program, mod_),
    MLUA_SYM_F(get_unique_id, mod_),
    MLUA_SYM_F(do_cmd, mod_),
};

MLUA_OPEN_MODULE(hardware.flash) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
