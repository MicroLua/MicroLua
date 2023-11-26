// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <string.h>

#include "hardware/flash.h"
#include "hardware/sync.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/block.h"
#include "mlua/errors.h"
#include "mlua/module.h"
#include "mlua/util.h"

// TODO: Use pico_flash instead of hardware_flash to handle lockout

extern char const __flash_binary_start;

typedef struct Flash {
    MLuaBlockDev dev;
    char const* start;
} Flash;

static int flash_dev_read(MLuaBlockDev* dev, uint64_t off, void* dst,
                          size_t size) {
    Flash* f = (Flash*)dev;
    if (off + size > f->dev.size) return MLUA_EINVAL;
    memcpy(dst, f->start + off, size);
    return 0;
}

static int flash_dev_write(MLuaBlockDev* dev, uint64_t off, void const* src,
                           size_t size) {
    Flash* f = (Flash*)dev;
    if (off + size > f->dev.size) return MLUA_EINVAL;
    uint32_t save = save_and_disable_interrupts();
    flash_range_program((f->start + off) - &__flash_binary_start, src, size);
    restore_interrupts(save);
    return 0;
}

static int flash_dev_erase(MLuaBlockDev* dev, uint64_t off, size_t size) {
    Flash* f = (Flash*)dev;
    if (off + size > f->dev.size) return MLUA_EINVAL;
    uint32_t save = save_and_disable_interrupts();
    flash_range_erase((f->start + off) - &__flash_binary_start, size);
    restore_interrupts(save);
    return 0;
}

static int flash_dev_sync(MLuaBlockDev* dev) {
    return 0;
}

static int mod_new(lua_State* ls) {
    uint32_t off = luaL_checkinteger(ls, 1);
    size_t size = luaL_checkinteger(ls, 2);

    // Align off and size to sector boundaries.
    uint32_t end = (off + size) & ~(FLASH_SECTOR_SIZE - 1);
    off = (off + FLASH_SECTOR_SIZE - 1) & ~(FLASH_SECTOR_SIZE - 1);
    if (off >= end) return luaL_error(ls, "zero-size device");
#ifdef PICO_FLASH_SIZE_BYTES
    if (end - off > PICO_FLASH_SIZE_BYTES) return luaL_error(ls, "too large");
#endif

    Flash* dev = (Flash*)mlua_new_BlockDev(ls, sizeof(Flash), 0);
    dev->dev.read = &flash_dev_read,
    dev->dev.write = &flash_dev_write,
    dev->dev.erase = &flash_dev_erase,
    dev->dev.sync = &flash_dev_sync,
    dev->dev.size = end - off;
    dev->dev.read_size = 1;
    dev->dev.write_size = FLASH_PAGE_SIZE;
    dev->dev.erase_size = FLASH_SECTOR_SIZE;
    dev->start = &__flash_binary_start + off;
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(new, mod_),
};

MLUA_OPEN_MODULE(mlua.block.flash) {
    mlua_require(ls, "mlua.block", false);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
