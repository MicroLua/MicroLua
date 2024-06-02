// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "pico/bootrom.h"

#include "lauxlib.h"
#include "mlua/module.h"
#include "mlua/util.h"

static void mod_VERSION(lua_State* ls, MLuaSymVal const* value) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
    lua_pushinteger(ls, *(uint8_t const*)(uintptr_t)0x00000013);
#pragma GCC diagnostic pop
}

int mod_table_code(lua_State* ls) {
    size_t len;
    char const* c = luaL_checklstring(ls, 1, &len);
    luaL_argcheck(ls, len == 2, 1, "2-character string");
    return lua_pushinteger(ls, rom_table_code(c[0], c[1])), 1;
}

int rom_lookup(lua_State* ls, void* (fn)(uint32_t)) {
    int top = lua_gettop(ls);
    for (int i = 1; i <= top; ++i) {
        uint32_t code = luaL_checkinteger(ls, i);
        lua_pushlightuserdata(ls, fn(code));
        lua_replace(ls, i);
    }
    return top;
}

int mod_func_lookup(lua_State* ls) {
    return rom_lookup(ls, &rom_func_lookup);
}

int mod_data_lookup(lua_State* ls) {
    return rom_lookup(ls, &rom_data_lookup);
}

MLUA_FUNC_V2(mod_,, reset_usb_boot, luaL_checkinteger, luaL_checkinteger)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(START, lightuserdata, (void*)(uintptr_t)0x00000000),
    MLUA_SYM_V(SIZE, integer, 16 << 10),
    MLUA_SYM_P(VERSION, mod_),
    MLUA_SYM_V(FUNC_POPCOUNT32, integer, ROM_FUNC_POPCOUNT32),
    MLUA_SYM_V(FUNC_REVERSE32, integer, ROM_FUNC_REVERSE32),
    MLUA_SYM_V(FUNC_CLZ32, integer, ROM_FUNC_CLZ32),
    MLUA_SYM_V(FUNC_CTZ32, integer, ROM_FUNC_CTZ32),
    MLUA_SYM_V(FUNC_MEMSET, integer, ROM_FUNC_MEMSET),
    MLUA_SYM_V(FUNC_MEMSET4, integer, ROM_FUNC_MEMSET4),
    MLUA_SYM_V(FUNC_MEMCPY, integer, ROM_FUNC_MEMCPY),
    MLUA_SYM_V(FUNC_MEMCPY44, integer, ROM_FUNC_MEMCPY44),
    MLUA_SYM_V(FUNC_RESET_USB_BOOT, integer, ROM_FUNC_RESET_USB_BOOT),
    MLUA_SYM_V(FUNC_CONNECT_INTERNAL_FLASH, integer, ROM_FUNC_CONNECT_INTERNAL_FLASH),
    MLUA_SYM_V(FUNC_FLASH_EXIT_XIP, integer, ROM_FUNC_FLASH_EXIT_XIP),
    MLUA_SYM_V(FUNC_FLASH_RANGE_ERASE, integer, ROM_FUNC_FLASH_RANGE_ERASE),
    MLUA_SYM_V(FUNC_FLASH_RANGE_PROGRAM, integer, ROM_FUNC_FLASH_RANGE_PROGRAM),
    MLUA_SYM_V(FUNC_FLASH_FLUSH_CACHE, integer, ROM_FUNC_FLASH_FLUSH_CACHE),
    MLUA_SYM_V(FUNC_FLASH_ENTER_CMD_XIP, integer, ROM_FUNC_FLASH_ENTER_CMD_XIP),
    MLUA_SYM_V(INTF_MASS_STORAGE, integer, 1),
    MLUA_SYM_V(INTF_PICOBOOT, integer, 2),

    MLUA_SYM_F(table_code, mod_),
    MLUA_SYM_F(func_lookup, mod_),
    MLUA_SYM_F(data_lookup, mod_),
    // rom_funcs_lookup: Use func_lookup()
    MLUA_SYM_F(reset_usb_boot, mod_),
};

MLUA_OPEN_MODULE(pico.bootrom) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
