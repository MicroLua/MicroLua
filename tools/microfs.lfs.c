// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "lfs.h"
#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

static char const* lfs_err(int err) {
    switch (err) {
    case LFS_ERR_IO: return "input / output error";
    case LFS_ERR_CORRUPT: return "corrupted";
    case LFS_ERR_NOENT: return "no such file or directory";
    case LFS_ERR_EXIST: return "file exists";
    case LFS_ERR_NOTDIR: return "not a directory";
    case LFS_ERR_ISDIR: return "is a directory";
    case LFS_ERR_NOTEMPTY: return "directory not empty";
    case LFS_ERR_BADF: return "bad file descriptor";
    case LFS_ERR_FBIG: return "file too large";
    case LFS_ERR_INVAL: return "invalid argument";
    case LFS_ERR_NOSPC: return "no space left on device";
    case LFS_ERR_NOMEM: return "no memory available";
    case LFS_ERR_NOATTR: return "no data / attr available";
    case LFS_ERR_NAMETOOLONG: return "filename too long";
    default: return "unknown error";
    }
}

static int lfs_error(lua_State* ls, int err) {
    lua_pushstring(ls, lfs_err(err));
    return lua_error(ls);
}

static char const Filesystem_name[] = "lfs.Filesystem";
static char const Dir_name[] = "lfs.Dir";

#define LOOKAHEAD_SIZE 16

typedef struct Filesystem {
    struct lfs_config config;
    lfs_t lfs;
    uint8_t* data;
    uint32_t lookahead_buffer[LOOKAHEAD_SIZE / sizeof(uint32_t)];
    uint8_t buffers[0];
} Filesystem;

static int fs_read(struct lfs_config const* c, lfs_block_t block,
                   lfs_off_t off, void* buffer, lfs_size_t size) {
    if (c->context == NULL) return LFS_ERR_IO;
    Filesystem* fs = c->context;
    memcpy(buffer, &fs->data[block * c->block_size + off], size);
    return LFS_ERR_OK;
}

static int fs_prog(struct lfs_config const* c, lfs_block_t block,
                   lfs_off_t off, void const* buffer, lfs_size_t size) {
    if (c->context == NULL) return LFS_ERR_IO;
    Filesystem* fs = c->context;
    memcpy(&fs->data[block * c->block_size + off], buffer, size);
    return LFS_ERR_OK;
}

static int fs_erase(struct lfs_config const* c, lfs_block_t block) {
    if (c->context == NULL) return LFS_ERR_IO;
    Filesystem* fs = c->context;
    memset(&fs->data[block * c->block_size], 0xff, c->block_size);
    return LFS_ERR_OK;
}

static int fs_sync(struct lfs_config const* c) {
    return LFS_ERR_OK;
}

static inline lfs_dir_t* check_Dir(lua_State* ls, int arg) {
    return luaL_checkudata(ls, arg, Dir_name);
}

static inline Filesystem* check_Filesystem(lua_State* ls, int arg) {
    return luaL_checkudata(ls, arg, Filesystem_name);
}

static Filesystem* check_mounted_Filesystem(lua_State* ls, int arg) {
    Filesystem* fs = check_Filesystem(ls, arg);
    if (fs->config.context == NULL) {
        return luaL_error(ls, "filesystem isn't mounted"), NULL;
    }
    return fs;
}

static int Dir___close(lua_State* ls) {
    lfs_dir_t* dir = check_Dir(ls, 1);
    if (lua_getiuservalue(ls, 1, 1) == LUA_TNIL) return 0;
    Filesystem* fs = check_mounted_Filesystem(ls, -1);
    lua_pushnil(ls);  // Mark as closed
    lua_setiuservalue(ls, 1, 1);
    lfs_dir_close(&fs->lfs, dir);  // Ignore error on purpose
    return 0;
}

static int Dir_next(lua_State* ls) {
    lfs_dir_t* dir = check_Dir(ls, 1);
    if (lua_getiuservalue(ls, 1, 1) == LUA_TNIL) return 0;
    Filesystem* fs = check_mounted_Filesystem(ls, -1);
    lua_pop(ls, 1);
    struct lfs_info info;
    int res = lfs_dir_read(&fs->lfs, dir, &info);
    if (res < 0) return lfs_error(ls, res);
    if (res == LFS_ERR_OK) return 0;
    lua_pushstring(ls, info.name);
    lua_pushinteger(ls, info.type);
    lua_pushinteger(ls, info.size);
    return 3;
}

MLUA_SYMBOLS_NOHASH(Dir_syms) = {
    MLUA_SYM_F(__close, Dir_),
};

static int Filesystem___close(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (fs->config.context == NULL) return 0;
    int res = lfs_unmount(&fs->lfs);
    fs->config.context = NULL;
    if (res < 0) return lfs_error(ls, res);
    return 0;
}

static int Filesystem_unmount(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (fs->config.context != NULL) {
        int res = lfs_unmount(&fs->lfs);
        fs->config.context = NULL;
        if (res < 0) return lfs_error(ls, res);
    }
    lua_pushlstring(ls, (char const*)fs->data,
                    lua_rawlen(ls, 1) - sizeof(*fs) - 2 * fs->config.prog_size);
    return 1;
}

static int Filesystem_list(lua_State* ls) {
    Filesystem* fs = check_mounted_Filesystem(ls, 1);
    char const* path = luaL_checkstring(ls, 2);
    lua_pushcfunction(ls, &Dir_next);  // Iterator function
    lfs_dir_t* dir = lua_newuserdatauv(ls, sizeof(lfs_dir_t), 1);  // State
    int res = lfs_dir_open(&fs->lfs, dir, path);
    if (res < 0) return lfs_error(ls, res);
    luaL_getmetatable(ls, Dir_name);
    lua_setmetatable(ls, -2);
    lua_pushvalue(ls, 1);
    lua_setiuservalue(ls, -2, 1);
    lua_pushnil(ls);  // Control variable
    lua_pushvalue(ls, -2);  // Closing value
    return 4;
}

static int Filesystem_read(lua_State* ls) {
    Filesystem* fs = check_mounted_Filesystem(ls, 1);
    char const* path = luaL_checkstring(ls, 2);
    lfs_file_t file;
    int res = lfs_file_open(&fs->lfs, &file, path, LFS_O_RDONLY);
    if (res < 0) return lfs_error(ls, res);
    lfs_soff_t size = lfs_file_size(&fs->lfs, &file);
    luaL_Buffer buf;
    void* dst = luaL_buffinitsize(ls, &buf, size);
    lfs_ssize_t rres = lfs_file_read(&fs->lfs, &file, dst, size);
    res = lfs_file_close(&fs->lfs, &file);
    if (rres < 0) return lfs_error(ls, rres);
    if (res < 0) return lfs_error(ls, res);
    return luaL_pushresultsize(&buf, rres), 1;
}

static int Filesystem_write(lua_State* ls) {
    Filesystem* fs = check_mounted_Filesystem(ls, 1);
    char const* path = luaL_checkstring(ls, 2);
    size_t size;
    void const* src = luaL_checklstring(ls, 3, &size);
    lfs_file_t file;
    int res = lfs_file_open(&fs->lfs, &file, path,
                            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (res < 0) return lfs_error(ls, res);
    lfs_ssize_t wres = lfs_file_write(&fs->lfs, &file, src, size);
    res = lfs_file_close(&fs->lfs, &file);
    if (wres < 0) return lfs_error(ls, wres);
    if (res < 0) return lfs_error(ls, res);
    return 0;
}

static int Filesystem_mkdir(lua_State* ls) {
    Filesystem* fs = check_mounted_Filesystem(ls, 1);
    char const* path = luaL_checkstring(ls, 2);
    int res = lfs_mkdir(&fs->lfs, path);
    if (res < 0) return lfs_error(ls, res);
    return 0;
}

static int Filesystem_remove(lua_State* ls) {
    Filesystem* fs = check_mounted_Filesystem(ls, 1);
    char const* path = luaL_checkstring(ls, 2);
    int res = lfs_remove(&fs->lfs, path);
    if (res < 0) return lfs_error(ls, res);
    return 0;
}

MLUA_SYMBOLS_NOHASH(Filesystem_syms) = {
    MLUA_SYM_F(__close, Filesystem_),
    MLUA_SYM_F(unmount, Filesystem_),
    MLUA_SYM_F(list, Filesystem_),
    MLUA_SYM_F(read, Filesystem_),
    MLUA_SYM_F(write, Filesystem_),
    MLUA_SYM_F(mkdir, Filesystem_),
    MLUA_SYM_F(remove, Filesystem_),
};

static int mod_new(lua_State* ls) {
    size_t size;
    char const* data = NULL;
    if (lua_isinteger(ls, 1)) {
        size = lua_tointeger(ls, 1);
    } else if (lua_isstring(ls, 1)) {
        data = lua_tolstring(ls, 1, &size);
    } else {
        return luaL_typeerror(ls, 1, "integer or string");
    }
    uint32_t write_size = luaL_optinteger(ls, 2, 256);
    uint32_t erase_size = luaL_optinteger(ls, 3, 4096);
    size -= size % erase_size;

    Filesystem* fs = lua_newuserdatauv(
        ls, sizeof(Filesystem) + 2 * write_size + size, 0);
    luaL_getmetatable(ls, Filesystem_name);
    lua_setmetatable(ls, -2);
    memset(fs, 0, sizeof(*fs));
    fs->config.context = fs;
    fs->config.read = &fs_read;
    fs->config.prog = &fs_prog;
    fs->config.erase = &fs_erase;
    fs->config.sync = &fs_sync;
    fs->config.read_size = 1;
    fs->config.prog_size = write_size;
    fs->config.block_size = erase_size;
    fs->config.block_cycles = -1;
    fs->config.cache_size = fs->config.prog_size;
    fs->config.lookahead_size = LOOKAHEAD_SIZE;
    fs->config.read_buffer = fs->buffers;
    fs->config.prog_buffer = &fs->buffers[fs->config.cache_size];
    fs->config.lookahead_buffer = fs->lookahead_buffer;
    fs->data = &fs->buffers[2 * fs->config.cache_size];
    if (data != NULL) {
        memcpy(fs->data, data, size);
    } else {
        memset(fs->data, 0xff, size);
        fs->config.block_count = size / fs->config.block_size;
        int res = lfs_format(&fs->lfs, &fs->config);
        if (res < 0) return lfs_error(ls, res);
    }
    int res = lfs_mount(&fs->lfs, &fs->config);
    if (res < 0) return lfs_error(ls, res);
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(new, mod_),
    MLUA_SYM_V(TYPE_REG, integer, LFS_TYPE_REG),
    MLUA_SYM_V(TYPE_DIR, integer, LFS_TYPE_DIR),
};

MLUA_OPEN_MODULE(microfs.lfs) {
    // Create the Dir class.
    // TODO: Correctly handle hash vs. nohash
    mlua_new_class(ls, Dir_name, Dir_syms, true);
    lua_pop(ls, 1);

    // Create the Filesystem class.
    mlua_new_class(ls, Filesystem_name, Filesystem_syms, true);
    lua_pop(ls, 1);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
