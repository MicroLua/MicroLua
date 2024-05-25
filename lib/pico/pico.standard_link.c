// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

#define EXTERN_SYM(n, en) \
extern char const en[]; \
static void mod_ ## n(lua_State* ls, MLuaSymVal const* value) { \
    lua_pushlightuserdata(ls, (void*)en); \
}

EXTERN_SYM(flash_binary_start, __flash_binary_start)
EXTERN_SYM(flash_binary_end, __flash_binary_end)
EXTERN_SYM(logical_binary_start, __logical_binary_start)
EXTERN_SYM(binary_info_header_end, __binary_info_header_end)
EXTERN_SYM(binary_info_start, __binary_info_start)
EXTERN_SYM(binary_info_end, __binary_info_end)
EXTERN_SYM(ram_vector_table, ram_vector_table)
EXTERN_SYM(data_source, __etext)
EXTERN_SYM(data_start, __data_start__)
EXTERN_SYM(data_end, __data_end__)
EXTERN_SYM(bss_start, __bss_start__)
EXTERN_SYM(bss_end, __bss_end__)
EXTERN_SYM(scratch_x_source, __scratch_x_source__)
EXTERN_SYM(scratch_x_start, __scratch_x_start__)
EXTERN_SYM(scratch_x_end, __scratch_x_end__)
EXTERN_SYM(scratch_y_source, __scratch_y_source__)
EXTERN_SYM(scratch_y_start, __scratch_y_start__)
EXTERN_SYM(scratch_y_end, __scratch_y_end__)
EXTERN_SYM(heap_start, __end__)
EXTERN_SYM(heap_limit, __HeapLimit)
EXTERN_SYM(heap_end, __StackLimit)
EXTERN_SYM(stack1_bottom, __StackOneBottom)
EXTERN_SYM(stack1_top, __StackOneTop)
EXTERN_SYM(stack_bottom, __StackBottom)
EXTERN_SYM(stack_top, __StackTop)

#define mod_FLASH mod_flash_binary_start
#define mod_RAM mod_ram_vector_table
#define mod_SCRATCH_X mod_scratch_x_start
#define mod_SCRATCH_Y mod_scratch_y_start

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_P(FLASH, mod_),
    MLUA_SYM_V(FLASH_SIZE, integer, 2 << 20),
    MLUA_SYM_P(RAM, mod_),
    MLUA_SYM_V(RAM_SIZE, integer, 256 << 10),
    MLUA_SYM_P(SCRATCH_X, mod_),
    MLUA_SYM_V(SCRATCH_X_SIZE, integer, 4 << 10),
    MLUA_SYM_P(SCRATCH_Y, mod_),
    MLUA_SYM_V(SCRATCH_Y_SIZE, integer, 4 << 10),
    MLUA_SYM_P(flash_binary_start, mod_),
    MLUA_SYM_P(flash_binary_end, mod_),
    MLUA_SYM_P(logical_binary_start, mod_),
    MLUA_SYM_P(binary_info_header_end, mod_),
    MLUA_SYM_P(binary_info_start, mod_),
    MLUA_SYM_P(binary_info_end, mod_),
    MLUA_SYM_P(ram_vector_table, mod_),
    MLUA_SYM_P(data_source, mod_),
    MLUA_SYM_P(data_start, mod_),
    MLUA_SYM_P(data_end, mod_),
    MLUA_SYM_P(bss_start, mod_),
    MLUA_SYM_P(bss_end, mod_),
    MLUA_SYM_P(scratch_x_source, mod_),
    MLUA_SYM_P(scratch_x_start, mod_),
    MLUA_SYM_P(scratch_x_end, mod_),
    MLUA_SYM_P(scratch_y_source, mod_),
    MLUA_SYM_P(scratch_y_start, mod_),
    MLUA_SYM_P(scratch_y_end, mod_),
    MLUA_SYM_P(heap_start, mod_),
    MLUA_SYM_P(heap_limit, mod_),
    MLUA_SYM_P(heap_end, mod_),
    MLUA_SYM_P(stack1_bottom, mod_),
    MLUA_SYM_P(stack1_top, mod_),
    MLUA_SYM_P(stack_bottom, mod_),
    MLUA_SYM_P(stack_top, mod_),
};

MLUA_OPEN_MODULE(pico.standard_link) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
