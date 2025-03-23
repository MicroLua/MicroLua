// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <assert.h>
#include <string.h>

#include "hardware/flash.h"
#include "pico.h"
#include "pico/binary_info.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/errors.h"
#include "mlua/block.flash.h"
#include "mlua/fs.h"
#include "mlua/fs.lfs.h"
#include "mlua/module.h"
#include "mlua/util.h"

#ifndef MLUA_FS_LOADER_OFFSET
#define MLUA_FS_LOADER_OFFSET (PICO_FLASH_SIZE_BYTES - (MLUA_FS_LOADER_SIZE))
#endif
#ifndef MLUA_FS_LOADER_SIZE
#define MLUA_FS_LOADER_SIZE (1u << 20)
#endif
#ifndef MLUA_FS_LOADER_BASE
#define MLUA_FS_LOADER_BASE "/lua"
#endif
#ifndef MLUA_FS_LOADER_FLAT
#define MLUA_FS_LOADER_FLAT 1
#endif

static_assert(((MLUA_FS_LOADER_OFFSET) & (FLASH_SECTOR_SIZE - 1)) == 0,
              "MLUA_FS_LOADER_OFFSET must be a multiple of FLASH_SECTOR_SIZE");
static_assert(((MLUA_FS_LOADER_SIZE) & (FLASH_SECTOR_SIZE - 1)) == 0,
              "MLUA_FS_LOADER_SIZE must be a multiple of FLASH_SECTOR_SIZE");
static_assert(
    (MLUA_FS_LOADER_OFFSET) >= 0
    && ((MLUA_FS_LOADER_OFFSET) + (MLUA_FS_LOADER_SIZE) <= PICO_FLASH_SIZE_BYTES),
    "Filesystem is outside of flash boundaries");

bi_decl(bi_block_device(
    MLUA_BI_TAG, "lfs:loader",
    XIP_BASE + (MLUA_FS_LOADER_OFFSET), (MLUA_FS_LOADER_SIZE), NULL,
    BINARY_INFO_BLOCK_DEV_FLAG_READ | BINARY_INFO_BLOCK_DEV_FLAG_WRITE
    | BINARY_INFO_BLOCK_DEV_FLAG_REFORMAT | BINARY_INFO_BLOCK_DEV_FLAG_PT_NONE))

extern char const __flash_binary_start[];
extern char const __flash_binary_end[];

static MLuaBlockFlash dev;
static void* fs;

static __attribute__((constructor)) void init(void) {
    mlua_block_flash_init(&dev, (MLUA_FS_LOADER_OFFSET), (MLUA_FS_LOADER_SIZE));
    if (__flash_binary_end > __flash_binary_start + (MLUA_FS_LOADER_OFFSET)) {
        return;  // Filesystem overlaps with binary
    }
    fs = mlua_fs_lfs_alloc(&dev.dev);  // Never freed
    mlua_fs_lfs_mount(fs);
}

typedef struct Loader {
    void* buffer;
    int file;
    lua_Integer err;
} Loader;

static char const* load_block(lua_State* ls, void* buf, size_t* size) {
    Loader* l = buf;
    lua_getfield(ls, l->file, "read");
    lua_pushvalue(ls, l->file);
    lua_pushinteger(ls, sizeof(luaL_Buffer));
    if (lua_pcall(ls, 2, 3, 0) != LUA_OK) {
        l->err = MLUA_EIO;
        return lua_pop(ls, 1), NULL;
    } else if (!lua_toboolean(ls, -3)) {
        l->err = lua_tointeger(ls, -1);
        return lua_pop(ls, 3), NULL;
    }
    char const* data = lua_tolstring(ls, -3, size);
    memcpy(l->buffer, data, *size);
    lua_pop(ls, 3);
    return l->buffer;
}

static int load_module(lua_State* ls) {
    lua_pushvalue(ls, lua_upvalueindex(1));
    lua_rotate(ls, 1, 1);
    mlua_new_lua_module(ls, lua_tostring(ls, lua_upvalueindex(2)));
    if (!lua_setupvalue(ls, 1, 1)) lua_pop(ls, 1);  // Set _ENV
    lua_call(ls, lua_gettop(ls) - 1, 1);
    return 1;
}

static int mod_search(lua_State* ls) {
    char const* mod = luaL_checkstring(ls, 1);
    if (!MLUA_FS_LOADER_FLAT && strchr(mod, '.') != NULL) {
        mod = luaL_gsub(ls, mod, ".", LUA_DIRSEP);
    }

    // Get the filesystem.
    if (lua_getfield(ls, lua_upvalueindex(1), "fs") == LUA_TNIL) {
        return lua_pushliteral(ls, "no filesystem"), 1;
    }
    int fs_index = lua_gettop(ls);
    char const* err = "";
    char const* esep = err;

    // Compute the search path.
    mlua_require(ls, "package", true);
    lua_getfield(ls, -1, "path");
    luaL_Buffer buf;
    luaL_buffinit(ls, &buf);
    luaL_addgsub(&buf, lua_tostring(ls, fs_index + 2), LUA_PATH_MARK, mod);
    luaL_addchar(&buf, *LUA_PATH_SEP);

    // Try each file name in the search path.
    char* path = luaL_buffaddr(&buf);
    char* end = path + luaL_bufflen(&buf);
    for (;;) {
        if (path == end) return lua_settop(ls, fs_index + 1), 1;
        char* sep = strchr(path, *LUA_PATH_SEP);
        *sep = '\0';

        // Try to open the file.
        lua_getfield(ls, fs_index, "open");
        lua_pushvalue(ls, fs_index);
        lua_pushstring(ls, path);
        lua_pushinteger(ls, MLUA_FS_O_RDONLY);
        if (lua_pcall(ls, 3, 2, 0) != LUA_OK) {  // open() raised an error
            err = lua_pushfstring(ls, "%s%s%s: %s", err, esep, path,
                                  lua_tostring(ls, -1));
            lua_replace(ls, fs_index + 1);
            esep = "\n\t";
            lua_pop(ls, 1);
        } else if (!lua_toboolean(ls, -2)) {  // open() failed
            err = lua_pushfstring(ls, "%s%s%s: %s", err, esep, path,
                                  lua_tostring(ls, -1));
            lua_replace(ls, fs_index + 1);
            esep = "\n\t";
            lua_pop(ls, 2);
        } else {  // open() succeeded
            lua_pop(ls, 1);
            lua_replace(ls, fs_index + 1);  // File
            lua_pushstring(ls, path);
            lua_replace(ls, fs_index + 2);  // Path
            lua_settop(ls, fs_index + 2);  // Closes buf
            lua_toclose(ls, fs_index + 1);
            break;
        }
        path = sep + 1;
    }

    // Load the module from the file. Re-use buf as a data buffer.
    Loader l = {.buffer = &buf, .file = fs_index + 1, .err = MLUA_EOK};
    lua_pushfstring(ls, "@%s", lua_tostring(ls, fs_index + 2));
    if (lua_load(ls, &load_block, &l, lua_tostring(ls, -1), NULL) != LUA_OK) {
        return 1;
    } else if (l.err != MLUA_EOK) {
        lua_pop(ls, 2);
        return lua_pushfstring(ls, "%s: %s", lua_tostring(ls, fs_index + 2),
                               mlua_err_msg(l.err)), 1;
    }
    lua_pushvalue(ls, 1);  // Module name
    lua_pushcclosure(ls, &load_module, 2);
    lua_pushvalue(ls, fs_index + 2);
    return 2;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(block, boolean, false),
    MLUA_SYM_V(fs, boolean, false),
};

MLUA_OPEN_MODULE(mlua.fs.loader) {
    // Create the module.
    mlua_new_module(ls, 0, module_syms);
    int mod_index = lua_gettop(ls);

    // Update package.path.
    mlua_require(ls, "package", true);
    lua_getfield(ls, -1, "path");
    lua_pushfstring(ls,
        "%s%s" MLUA_FS_LOADER_BASE "/?.lua"
#if !MLUA_FS_LOADER_FLAT
        ";" MLUA_FS_LOADER_BASE "/?/init.lua"
#endif
        , lua_tostring(ls, -1), luaL_len(ls, -1) > 0 ? ";" : "");
    lua_setfield(ls, -3, "path");
    lua_pop(ls, 1);  // Remove path

    // Set up the module searcher.
    lua_getfield(ls, -1, "searchers");
    lua_pushvalue(ls, mod_index);
    lua_pushcclosure(ls, &mod_search, 1);
    lua_seti(ls, -2, luaL_len(ls, -2) + 1);
    lua_pop(ls, 2);  // Remove searchers & package

    // Create a reference to the global block device.
    mlua_require(ls, "mlua.block.flash", false);
    *((MLuaBlockFlash**)mlua_block_push(ls, sizeof(MLuaBlockFlash*), 0)) = &dev;
    lua_setfield(ls, mod_index, "block");

    // Create a reference to the global filesystem.
    if (fs != NULL) {
        mlua_require(ls, "mlua.fs.lfs", false);
        mlua_fs_lfs_push(ls, fs);
        lua_setfield(ls, mod_index, "fs");
    }

    return lua_settop(ls, mod_index), 1;
}
