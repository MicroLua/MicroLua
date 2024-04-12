// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/fs.lfs.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef LFS_THREADSAFE
#include "pico/mutex.h"
#endif

#include "lfs.h"
#include "lua.h"
#include "lauxlib.h"
#include "mlua/errors.h"
#include "mlua/fs.h"
#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/util.h"

static char const Filesystem_name[] = "mlua.fs.lfs.Filesystem";
static char const File_name[] = "mlua.fs.lfs.File";
static char const Dir_name[] = "mlua.fs.lfs.Dir";

#define LOOKAHEAD_SIZE 16

typedef struct Filesystem {
    struct lfs_config config;
    lfs_t lfs;
#ifdef LFS_THREADSAFE
    mutex_t mu;
#endif
    bool mounted;
    uint32_t lookahead_buffer[LOOKAHEAD_SIZE / sizeof(uint32_t)];
    uint8_t buffers[0];
} Filesystem;

typedef struct File {
    lfs_file_t file;
    struct lfs_file_config config;
    uint8_t buffer[0];
} File;

typedef struct Dir {
    lfs_dir_t dir;
} Dir;

static inline MLuaBlockDev* fs_dev(Filesystem* fs) {
    return fs->config.context;
}

static int mlua_err(int err) {
    switch (err) {
    case LFS_ERR_IO: return MLUA_EIO;
    case LFS_ERR_CORRUPT: return MLUA_ECORRUPT;
    case LFS_ERR_NOENT: return MLUA_ENOENT;
    case LFS_ERR_EXIST: return MLUA_EEXIST;
    case LFS_ERR_NOTDIR: return MLUA_ENOTDIR;
    case LFS_ERR_ISDIR: return MLUA_EISDIR;
    case LFS_ERR_NOTEMPTY: return MLUA_ENOTEMPTY;
    case LFS_ERR_BADF: return MLUA_EBADF;
    case LFS_ERR_FBIG: return MLUA_EFBIG;
    case LFS_ERR_INVAL: return MLUA_EINVAL;
    case LFS_ERR_NOSPC: return MLUA_ENOSPC;
    case LFS_ERR_NOMEM: return MLUA_ENOMEM;
    case LFS_ERR_NOATTR: return MLUA_ENODATA;
    case LFS_ERR_NAMETOOLONG: return MLUA_ENAMETOOLONG;
    default: return err;
    }
}

static int to_file_type(int type) {
    switch (type) {
    case LFS_TYPE_REG: return MLUA_FS_TYPE_REG;
    case LFS_TYPE_DIR: return MLUA_FS_TYPE_DIR;
    default: return MLUA_FS_TYPE_UNKNOWN;
    }
}

static_assert(MLUA_FS_O_RDWR == (MLUA_FS_O_RDONLY | MLUA_FS_O_WRONLY),
              "inconsistent MLUA_FS_O_* flags");
#ifndef LFS_READONLY
static_assert(LFS_O_RDWR == (LFS_O_RDONLY | LFS_O_WRONLY),
              "inconsistent LFS_O_* flags");
#endif

static int from_open_flags(int flags) {
    int res = 0;
    if ((flags & MLUA_FS_O_RDONLY) != 0) res |= LFS_O_RDONLY;
#ifndef LFS_READONLY
    if ((flags & MLUA_FS_O_WRONLY) != 0) res |= LFS_O_WRONLY;
    if ((flags & MLUA_FS_O_CREAT) != 0) res |= LFS_O_CREAT;
    if ((flags & MLUA_FS_O_EXCL) != 0) res |= LFS_O_EXCL;
    if ((flags & MLUA_FS_O_TRUNC) != 0) res |= LFS_O_TRUNC;
    if ((flags & MLUA_FS_O_APPEND) != 0) res |= LFS_O_APPEND;
#endif
    return res;
}

static int from_seek_whence(int whence) {
    switch (whence) {
    case MLUA_FS_SEEK_SET: return LFS_SEEK_SET;
    case MLUA_FS_SEEK_CUR: return LFS_SEEK_CUR;
    case MLUA_FS_SEEK_END: return LFS_SEEK_END;
    default: return LFS_SEEK_SET;
    }
}

static int push_error(lua_State* ls, int err) {
    return mlua_err_push(ls, mlua_err(err));
}

static int push_lfs_result_bool(lua_State* ls, int res) {
    if (res < 0) return push_error(ls, res);
    return lua_pushboolean(ls, true), 1;
}

static int push_lfs_result_int(lua_State* ls, lua_Integer res) {
    if (res < 0) return push_error(ls, res);
    return lua_pushinteger(ls, res), 1;
}

static int fs_read(struct lfs_config const* c, lfs_block_t block,
                   lfs_off_t off, void* buffer, lfs_size_t size) {
    MLuaBlockDev* dev = c->context;
    return dev->read(dev, (uint64_t)block * c->block_size + off, buffer, size);
}

static int fs_prog(struct lfs_config const* c, lfs_block_t block,
                   lfs_off_t off, void const* buffer, lfs_size_t size) {
    MLuaBlockDev* dev = c->context;
    return dev->write(dev, (uint64_t)block * c->block_size + off, buffer, size);
}

static int fs_erase(struct lfs_config const* c, lfs_block_t block) {
    MLuaBlockDev* dev = c->context;
    return dev->erase(dev, (uint64_t)block * c->block_size, c->block_size);
}

static int fs_sync(struct lfs_config const* c) {
    MLuaBlockDev* dev = c->context;
    return dev->sync(dev);
}

#ifdef LFS_THREADSAFE

static int fs_lock(struct lfs_config const* c) {
    Filesystem* fs = (Filesystem*)c;
    mutex_enter_blocking(&fs->mu);
    return LFS_ERR_OK;
}

static int fs_unlock(struct lfs_config const* c) {
    Filesystem* fs = (Filesystem*)c;
    mutex_exit(&fs->mu);
    return LFS_ERR_OK;
}

#endif  // LFS_THREADSAFE

static void init_filesystem(Filesystem* fs, MLuaBlockDev* dev) {
    memset(fs, 0, sizeof(*fs));
    fs->config.context = dev;
    fs->config.read = &fs_read;
    fs->config.prog = &fs_prog;
    fs->config.erase = &fs_erase;
    fs->config.sync = &fs_sync;
#ifdef LFS_THREADSAFE
    mutex_init(&fs->mu);
    fs->config.lock = &fs_lock;
    fs->config.unlock = &fs_unlock;
#endif
    fs->config.read_size = dev->read_size;
    fs->config.prog_size = dev->write_size;
    fs->config.block_size = dev->erase_size;
    fs->config.block_cycles = 500;
    fs->config.cache_size = fs->config.prog_size;
    fs->config.lookahead_size = LOOKAHEAD_SIZE;
    fs->config.read_buffer = fs->buffers;
    fs->config.prog_buffer = &fs->buffers[fs->config.cache_size];
    fs->config.lookahead_buffer = fs->lookahead_buffer;
}

static inline Filesystem* check_Filesystem(lua_State* ls, int arg) {
    void* ptr = luaL_checkudata(ls, arg, Filesystem_name);
    if (ptr == NULL || lua_rawlen(ls, arg) != sizeof(ptr)) return ptr;
    return *((Filesystem**)ptr);
}

static inline Filesystem* to_Filesystem(lua_State* ls, int arg) {
    void* ptr = lua_touserdata(ls, arg);
    if (lua_rawlen(ls, arg) != sizeof(ptr)) return ptr;
    return *((Filesystem**)ptr);
}

static uint8_t check_attr(lua_State* ls, int arg) {
    lua_Unsigned attr = luaL_checkinteger(ls, arg);
    luaL_argcheck(ls, attr <= 0xff, arg, "invalid attribute type");
    return attr;
}

static lfs_size_t check_blocks(lua_State* ls, Filesystem* fs, int arg) {
    uint64_t dev_size = fs_dev(fs)->size;
    uint64_t size = lua_isnoneornil(ls, arg) ? dev_size
                    : (uint64_t)mlua_check_int64(ls, arg);
    luaL_argcheck(ls, size <= dev_size, arg, "larger than block device");
    uint64_t blocks = size / fs->config.block_size;
    luaL_argcheck(ls, blocks <= (lfs_size_t)-1, arg, "too large");
    return blocks;
}

static int Filesystem_format(lua_State* ls) {
#ifdef LFS_READONLY
    return mlua_err_push(ls, MLUA_EROFS);
#else
    Filesystem* fs = check_Filesystem(ls, 1);
    if (fs->mounted) return mlua_err_push(ls, MLUA_EBUSY);
    lfs_size_t blocks = check_blocks(ls, fs, 2);
    fs->config.block_count = blocks;
    return push_lfs_result_bool(ls, lfs_format(&fs->lfs, &fs->config));
#endif
}

static int Filesystem_mount(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (fs->mounted) return mlua_err_push(ls, MLUA_EBUSY);
    // TODO: Mounting can cause hard exceptions. Attempt to identify that a
    // filesystem is actually present before mounting, e.g. look for "littlefs".
    int res = lfs_mount(&fs->lfs, &fs->config);
    if (res < 0) return push_error(ls, res);
    fs->mounted = true;
    return lua_pushboolean(ls, true), 1;
}

static int Filesystem_unmount(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (fs->mounted) {
        fs->mounted = false;
        return push_lfs_result_bool(ls, lfs_unmount(&fs->lfs));
    }
    return lua_pushboolean(ls, true), 1;
}

static int Filesystem_is_mounted(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    return lua_pushboolean(ls, fs->mounted), 1;
}

static int Filesystem_grow(lua_State* ls) {
#ifdef LFS_READONLY
    return mlua_err_push(ls, MLUA_EROFS);
#else
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    lfs_size_t blocks = check_blocks(ls, fs, 2);
    struct lfs_fsinfo info;
    int res = lfs_fs_stat(&fs->lfs, &info);
    if (res < 0) return push_error(ls, res);
    luaL_argcheck(ls, blocks >= info.block_count, 2, "shrinking not supported");
    return push_lfs_result_bool(ls, lfs_fs_grow(&fs->lfs, blocks));
#endif
}

static int Filesystem_statvfs(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    struct lfs_fsinfo info;
    int res = lfs_fs_stat(&fs->lfs, &info);
    if (res < 0) return push_error(ls, res);
    lua_pushinteger(ls, info.disk_version);
    lua_pushinteger(ls, info.block_size);
    lua_pushinteger(ls, info.block_count);
    lua_pushinteger(ls, info.name_max);
    lua_pushinteger(ls, info.file_max);
    lua_pushinteger(ls, info.attr_max);
    return 6;
}

static int Filesystem_size(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    return push_lfs_result_int(ls, lfs_fs_size(&fs->lfs));
}

static int Filesystem_gc(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    return push_lfs_result_bool(ls, lfs_fs_gc(&fs->lfs));
}

static int fs_traverse(void* data, lfs_block_t block) {
    lua_State* ls = data;
    lua_pushvalue(ls, 2);
    lua_pushinteger(ls, block);
    lua_call(ls, 1, 1);
    return lua_tointeger(ls, -1);
}

static int Filesystem_traverse(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    return push_lfs_result_bool(ls,
        lfs_fs_traverse(&fs->lfs, &fs_traverse, ls));
}

static int Filesystem_mkconsistent(lua_State* ls) {
#ifdef LFS_READONLY
    return mlua_err_push(ls, MLUA_EROFS);
#else
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    return push_lfs_result_bool(ls, lfs_fs_mkconsistent(&fs->lfs));
#endif
}

static int Filesystem_open(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    char const* path = luaL_checkstring(ls, 2);
    int flags = luaL_checkinteger(ls, 3);

#ifdef LFS_READONLY
    if (flags & (MLUA_FS_O_WRONLY | MLUA_FS_O_CREAT | MLUA_FS_O_EXCL
                 | MLUA_FS_O_TRUNC | MLUA_FS_O_APPEND) != 0) {
        return mlua_err_push(ls, MLUA_EROFS);
    }
#endif

    File* f = lua_newuserdatauv(ls, sizeof(File) + fs_dev(fs)->write_size, 1);
    memset(f, 0, sizeof(File));
    f->config.buffer = f->buffer;
    int res = lfs_file_opencfg(&fs->lfs, &f->file, path, from_open_flags(flags),
                               &f->config);
    if (res < 0) return push_error(ls, res);
    luaL_getmetatable(ls, File_name);
    lua_setmetatable(ls, -2);
    lua_pushvalue(ls, 1);  // Keep fs alive
    lua_setiuservalue(ls, -2, 1);
    return 1;
}

static int Dir_next(lua_State* ls);

static int Filesystem_list(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    char const* path = luaL_checkstring(ls, 2);

    lua_pushcfunction(ls, &Dir_next);  // Iterator function
    Dir* d = lua_newuserdatauv(ls, sizeof(Dir), 1);  // State
    memset(d, 0, sizeof(Dir));
    int res = lfs_dir_open(&fs->lfs, &d->dir, path);
    if (res < 0) return push_error(ls, res);
    luaL_getmetatable(ls, Dir_name);
    lua_setmetatable(ls, -2);
    lua_pushvalue(ls, 1);  // Keep fs alive
    lua_setiuservalue(ls, -2, 1);
    lua_pushnil(ls);  // Control variable
    lua_pushvalue(ls, -2);  // Closing value
    return 4;
}

static int Filesystem_stat(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    char const* path = luaL_checkstring(ls, 2);
    struct lfs_info info;
    int res = lfs_stat(&fs->lfs, path, &info);
    if (res < 0) return push_error(ls, res);
    lua_pushstring(ls, info.name);
    lua_pushinteger(ls, to_file_type(info.type));
    return info.type == LFS_TYPE_REG ? lua_pushinteger(ls, info.size), 3 : 2;
}

static int Filesystem_getattr(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    char const* path = luaL_checkstring(ls, 2);
    uint8_t attr = check_attr(ls, 3);
    luaL_Buffer buf;
    luaL_buffinit(ls, &buf);
    lfs_size_t size = LUAL_BUFFERSIZE;
    for (;;) {
        char* dst = luaL_prepbuffsize(&buf, size);
        lfs_ssize_t res = lfs_getattr(&fs->lfs, path, attr, dst, size);
        if (res < 0) return push_error(ls, res);
        if ((lfs_size_t)res <= size) return luaL_pushresultsize(&buf, res), 1;
        size = res;
    }
}

static int Filesystem_setattr(lua_State* ls) {
#ifdef LFS_READONLY
    return mlua_err_push(ls, MLUA_EROFS);
#else
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    char const* path = luaL_checkstring(ls, 2);
    uint8_t attr = check_attr(ls, 3);
    size_t size;
    void const* value = luaL_checklstring(ls, 4, &size);
    return push_lfs_result_bool(ls,
        lfs_setattr(&fs->lfs, path, attr, value, size));
#endif
}

static int Filesystem_removeattr(lua_State* ls) {
#ifdef LFS_READONLY
    return mlua_err_push(ls, MLUA_EROFS);
#else
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    char const* path = luaL_checkstring(ls, 2);
    uint8_t attr = check_attr(ls, 3);
    return push_lfs_result_bool(ls, lfs_removeattr(&fs->lfs, path, attr));
#endif
}

static int Filesystem_mkdir(lua_State* ls) {
#ifdef LFS_READONLY
    return mlua_err_push(ls, MLUA_EROFS);
#else
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    char const* path = luaL_checkstring(ls, 2);
    return push_lfs_result_bool(ls, lfs_mkdir(&fs->lfs, path));
#endif
}

static int Filesystem_remove(lua_State* ls) {
#ifdef LFS_READONLY
    return mlua_err_push(ls, MLUA_EROFS);
#else
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    char const* path = luaL_checkstring(ls, 2);
    return push_lfs_result_bool(ls, lfs_remove(&fs->lfs, path));
#endif
}

static int Filesystem_rename(lua_State* ls) {
#ifdef LFS_READONLY
    return mlua_err_push(ls, MLUA_EROFS);
#else
    Filesystem* fs = check_Filesystem(ls, 1);
    if (!fs->mounted) return mlua_err_push(ls, MLUA_ENOTCONN);
    char const* old_path = luaL_checkstring(ls, 2);
    char const* new_path = luaL_checkstring(ls, 3);
    return push_lfs_result_bool(ls, lfs_rename(&fs->lfs, old_path, new_path));
#endif
}

#if !defined(LFS_READONLY) && defined(LFS_MIGRATE)

static int Filesystem_migrate(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (fs->mounted) return mlua_err_push(ls, MLUA_EBUSY);
    lfs_size_t blocks = check_blocks(ls, fs, 2);
    fs->config.block_count = blocks;
    return push_lfs_result_bool(ls, lfs_migrate(&fs->lfs, &fs->config));
}

#endif

MLUA_SYMBOLS(Filesystem_syms) = {
    MLUA_SYM_F(format, Filesystem_),
    MLUA_SYM_F(mount, Filesystem_),
    MLUA_SYM_F(unmount, Filesystem_),
    MLUA_SYM_F(is_mounted, Filesystem_),
    MLUA_SYM_F(grow, Filesystem_),
    MLUA_SYM_F(statvfs, Filesystem_),
    MLUA_SYM_F(size, Filesystem_),
    MLUA_SYM_F(gc, Filesystem_),
    MLUA_SYM_F(traverse, Filesystem_),
    MLUA_SYM_F(mkconsistent, Filesystem_),
    MLUA_SYM_F(open, Filesystem_),
    MLUA_SYM_F(list, Filesystem_),
    MLUA_SYM_F(stat, Filesystem_),
    MLUA_SYM_F(getattr, Filesystem_),
    MLUA_SYM_F(setattr, Filesystem_),
    MLUA_SYM_F(removeattr, Filesystem_),
    MLUA_SYM_F(mkdir, Filesystem_),
    MLUA_SYM_F(remove, Filesystem_),
    MLUA_SYM_F(rename, Filesystem_),
#if !defined(LFS_READONLY) && defined(LFS_MIGRATE)
    MLUA_SYM_F(migrate, Filesystem_),
#else
    MLUA_SYM_V(migrate, boolean, false),
#endif
};

#define Filesystem___close Filesystem_unmount

MLUA_SYMBOLS_NOHASH(Filesystem_syms_nh) = {
    MLUA_SYM_F_NH(__close, Filesystem_),
};

static inline File* check_File(lua_State* ls, int arg, Filesystem** fs) {
    File* f = luaL_checkudata(ls, arg, File_name);
    if (lua_getiuservalue(ls, 1, 1) == LUA_TNIL) {
        return luaL_error(ls, "file is closed"), NULL;
    }
    *fs = to_Filesystem(ls, -1);
    lua_pop(ls, 1);
    return f;
}

static int File_close(lua_State* ls) {
    File* f = luaL_checkudata(ls, 1, File_name);
    if (lua_getiuservalue(ls, 1, 1) == LUA_TNIL) {  // Already closed
        return lua_pushboolean(ls, true), 1;
    }
    Filesystem* fs = to_Filesystem(ls, -1);
    lua_pushnil(ls);  // Mark as closed
    lua_setiuservalue(ls, 1, 1);
    return push_lfs_result_bool(ls, lfs_file_close(&fs->lfs, &f->file));
}

static int File_sync(lua_State* ls) {
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    return push_lfs_result_bool(ls, lfs_file_sync(&fs->lfs, &f->file));
}

static int File_read(lua_State* ls) {
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    // TODO: Make size optional
    // TODO: Use file size as maximum buffer size
    lfs_size_t size = luaL_checkinteger(ls, 2);
    luaL_Buffer buf;
    void* dst = luaL_buffinitsize(ls, &buf, size);
    lfs_ssize_t res = lfs_file_read(&fs->lfs, &f->file, dst, size);
    if (res < 0) return push_error(ls, res);
    return luaL_pushresultsize(&buf, res), 1;
}

static int File_write(lua_State* ls) {
#ifdef LFS_READONLY
    return mlua_err_push(ls, MLUA_EROFS);
#else
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    size_t size;
    void const* src = luaL_checklstring(ls, 2, &size);
    lfs_ssize_t res = lfs_file_write(&fs->lfs, &f->file, src, size);
    if (res < 0) return push_error(ls, res);
    return lua_pushinteger(ls, res), 1;
#endif
}

static int File_seek(lua_State* ls) {
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    lfs_soff_t off = luaL_checkinteger(ls, 2);
    int whence = luaL_optinteger(ls, 3, MLUA_FS_SEEK_SET);
    return push_lfs_result_int(ls,
        lfs_file_seek(&fs->lfs, &f->file, off, from_seek_whence(whence)));
}

static int File_tell(lua_State* ls) {
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    return push_lfs_result_int(ls, lfs_file_tell(&fs->lfs, &f->file));
}

static int File_rewind(lua_State* ls) {
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    return push_lfs_result_int(ls, lfs_file_rewind(&fs->lfs, &f->file));
}

static int File_size(lua_State* ls) {
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    return push_lfs_result_int(ls, lfs_file_size(&fs->lfs, &f->file));
}

static int File_truncate(lua_State* ls) {
#ifdef LFS_READONLY
    return mlua_err_push(ls, MLUA_EROFS);
#else
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    lfs_off_t size = luaL_checkinteger(ls, 2);
    return push_lfs_result_bool(ls,
        lfs_file_truncate(&fs->lfs, &f->file, size));
#endif
}

MLUA_SYMBOLS(File_syms) = {
    MLUA_SYM_F(close, File_),
    MLUA_SYM_F(sync, File_),
    MLUA_SYM_F(read, File_),
    MLUA_SYM_F(write, File_),
    MLUA_SYM_F(seek, File_),
    MLUA_SYM_F(tell, File_),
    MLUA_SYM_F(rewind, File_),
    MLUA_SYM_F(size, File_),
    MLUA_SYM_F(truncate, File_),
};

#define File___close File_close
#define File___gc File_close

MLUA_SYMBOLS_NOHASH(File_syms_nh) = {
    MLUA_SYM_F_NH(__close, File_),
    MLUA_SYM_F_NH(__gc, File_),
};

static inline Dir* check_Dir(lua_State* ls, int arg, Filesystem** fs) {
    Dir* d = luaL_checkudata(ls, arg, Dir_name);
    if (lua_getiuservalue(ls, 1, 1) == LUA_TNIL) {
        return luaL_error(ls, "directory is closed"), NULL;
    }
    *fs = to_Filesystem(ls, -1);
    lua_pop(ls, 1);
    return d;
}

static int Dir___close(lua_State* ls) {
    Dir* d = luaL_checkudata(ls, 1, Dir_name);
    if (lua_getiuservalue(ls, 1, 1) == LUA_TNIL) {  // Already closed
        return lua_pushboolean(ls, true), 1;
    }
    Filesystem* fs = to_Filesystem(ls, -1);
    lua_pushnil(ls);  // Mark as closed
    lua_setiuservalue(ls, 1, 1);
    return push_lfs_result_bool(ls, lfs_dir_close(&fs->lfs, &d->dir));
}

static int Dir_next(lua_State* ls) {
    Filesystem* fs = NULL;
    Dir* d = check_Dir(ls, 1, &fs);
    struct lfs_info info;
    int res = lfs_dir_read(&fs->lfs, &d->dir, &info);
    if (res == 0) return 0;
    if (res < 0) {
        lua_pushstring(ls, mlua_err_msg(mlua_err(res)));
        return lua_error(ls);
    }
    lua_pushstring(ls, info.name);
    lua_pushinteger(ls, to_file_type(info.type));
    return info.type == LFS_TYPE_REG ? lua_pushinteger(ls, info.size), 3 : 2;
}

MLUA_SYMBOLS(Dir_syms) = {
};

#define Dir___gc Dir___close

MLUA_SYMBOLS_NOHASH(Dir_syms_nh) = {
    MLUA_SYM_F_NH(__close, Dir_),
    MLUA_SYM_F_NH(__gc, Dir_),
};

void* mlua_fs_lfs_alloc(MLuaBlockDev* dev) {
    Filesystem* fs = malloc(sizeof(Filesystem) + 2 * dev->write_size);
    init_filesystem(fs, dev);
    return fs;
}

int mlua_fs_lfs_mount(void* fs) {
    Filesystem* tfs = fs;
    int res = lfs_mount(&tfs->lfs, &tfs->config);
    if (res < 0) return mlua_err(res);
    tfs->mounted = true;
    return MLUA_EOK;
}

void mlua_fs_lfs_push(lua_State* ls, void* fs) {
    *((Filesystem**)lua_newuserdatauv(ls, sizeof(Filesystem*), 0)) = fs;
    luaL_getmetatable(ls, Filesystem_name);
    lua_setmetatable(ls, -2);
}

static int mod_new(lua_State* ls) {
    MLuaBlockDev* dev = mlua_block_check(ls, 1);
    Filesystem* fs = lua_newuserdatauv(
        ls, sizeof(Filesystem) + 2 * dev->write_size, 1);
    luaL_getmetatable(ls, Filesystem_name);
    lua_setmetatable(ls, -2);
    lua_pushvalue(ls, 1);  // Keep dev alive
    lua_setiuservalue(ls, -2, 1);
    init_filesystem(fs, dev);
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(VERSION, integer, LFS_VERSION),
    MLUA_SYM_V(DISK_VERSION, integer, LFS_DISK_VERSION),
    MLUA_SYM_V(NAME_MAX, integer, LFS_NAME_MAX),
    MLUA_SYM_V(FILE_MAX, integer, LFS_FILE_MAX),
    MLUA_SYM_V(ATTR_MAX, integer, LFS_ATTR_MAX),

    MLUA_SYM_F(new, mod_),
};

MLUA_OPEN_MODULE(mlua.fs.lfs) {
    mlua_require(ls, "mlua.int64", false);
    mlua_require(ls, "mlua.block", false);

    // Create the Filesystem class.
    mlua_new_class(ls, Filesystem_name, Filesystem_syms, Filesystem_syms_nh);
    lua_pop(ls, 1);

    // Create the File class.
    mlua_new_class(ls, File_name, File_syms, File_syms_nh);
    lua_pop(ls, 1);

    // Create the Dir class.
    mlua_new_class(ls, Dir_name, Dir_syms, Dir_syms_nh);
    lua_pop(ls, 1);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
