// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_MLUA_BLOCK_FLASH_H
#define _MLUA_LIB_PICO_MLUA_BLOCK_FLASH_H

#include "mlua/block.h"

#ifdef __cplusplus
extern "C" {
#endif

// A block device in QSPI flash.
typedef struct MLuaBlockFlash {
    MLuaBlockDev dev;
    void const* start;
} MLuaBlockFlash;

// Initialize a flash block device. "offset" must be aligned to a
// FLASH_SECTOR_SIZE boundary. "size" must be a multiple of FLASH_SECTOR_SIZE.
void mlua_block_flash_init(MLuaBlockFlash* dev, uint32_t offset, size_t size);

#ifdef __cplusplus
}
#endif

#endif
