// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/platform.h"

#include "hardware/flash.h"
#include "pico/time.h"

uint64_t mlua_platform_time_us(void) {
    return to_us_since_boot(get_absolute_time());
}

void mlua_platform_time_range(uint64_t* min, uint64_t* max) {
    *min = to_us_since_boot(nil_time);
    *max = to_us_since_boot(at_the_end_of_time);
}

extern char const __flash_binary_start[];
extern char const __flash_binary_end[];

static MLuaFlash const flash = {
    .address = (uintptr_t)__flash_binary_start,
    .size = PICO_FLASH_SIZE_BYTES,
    .write_size = FLASH_PAGE_SIZE,
    .erase_size = FLASH_SECTOR_SIZE,
};

MLuaFlash const* mlua_platform_flash(void) { return &flash; }

uintptr_t mlua_platform_binary_size(void) {
    return __flash_binary_end - __flash_binary_start;
}
