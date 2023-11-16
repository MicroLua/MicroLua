// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>
#include <string.h>

#include "lfs.h"
#include "lua.h"
#include "lauxlib.h"
#include "mlua/block.h"
#include "mlua/module.h"
#include "mlua/util.h"

static char const Filesystem_name[] = "mlua.fs.littlefs.Filesystem";
static char const File_name[] = "mlua.fs.littlefs.File";
static char const Dir_name[] = "mlua.fs.littlefs.Dir";

#define LOOKAHEAD_SIZE 16

typedef struct Filesystem {
    lfs_t lfs;
    struct lfs_config config;
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

static char const* error_msg(Filesystem* fs, int err) {
    switch (err) {
    case LFS_ERR_OK: return "no error";
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
    default: return ((MLuaBlockDev*)fs->config.context)->error(err);
    }
}

static int push_error(lua_State* ls, Filesystem* fs, lua_Integer err) {
    mlua_push_fail(ls, error_msg(fs, err));
    lua_pushinteger(ls, err);
    return 3;
}

static int push_lfs_result_bool(lua_State* ls, Filesystem* fs, int res) {
    if (res != LFS_ERR_OK) return push_error(ls, fs, res);
    return lua_pushboolean(ls, true), 1;
}

static int push_lfs_result_int(lua_State* ls, Filesystem* fs, lua_Integer res) {
    if (res < 0) return push_error(ls, fs, res);
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

static struct lfs_config config_base = {
    .read = &fs_read,
    .prog = &fs_prog,
    .erase = &fs_erase,
    .sync = &fs_sync,
    .block_cycles = 500,
    .lookahead_size = LOOKAHEAD_SIZE,
};

static inline Filesystem* check_Filesystem(lua_State* ls, int arg) {
    return luaL_checkudata(ls, arg, Filesystem_name);
}

static inline Filesystem* check_mounted_Filesystem(lua_State* ls, int arg) {
    Filesystem* fs = luaL_checkudata(ls, arg, Filesystem_name);
    if (!fs->mounted) return luaL_error(ls, "filesystem isn't mounted"), NULL;
    return fs;
}

static inline Filesystem* to_Filesystem(lua_State* ls, int arg) {
    return lua_touserdata(ls, arg);
}

static int Filesystem_mount(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (fs->mounted) return luaL_error(ls, "filesystem is already mounted");
    int res = lfs_mount(&fs->lfs, &fs->config);
    if (res != LFS_ERR_OK) return push_error(ls, fs, res);
    fs->mounted = true;
    return lua_pushboolean(ls, true), 1;
}

static int Filesystem_unmount(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (fs->mounted) {
        fs->mounted = false;
        return push_lfs_result_bool(ls, fs, lfs_unmount(&fs->lfs));
    }
    return lua_pushboolean(ls, true), 1;
}

static int Filesystem_open(lua_State* ls) {
    Filesystem* fs = check_mounted_Filesystem(ls, 1);
    char const* path = luaL_checkstring(ls, 2);
    int flags = luaL_checkinteger(ls, 3);

    File* f = lua_newuserdatauv(
        ls, sizeof(File) + ((MLuaBlockDev*)fs->config.context)->write_size, 1);
    luaL_getmetatable(ls, File_name);
    lua_setmetatable(ls, -2);
    lua_pushvalue(ls, 1);  // Keep fs alive
    lua_setiuservalue(ls, -2, 1);
    memset(f, 0, sizeof(File));
    f->config.buffer = f->buffer;

    int res = lfs_file_opencfg(&fs->lfs, &f->file, path, flags, &f->config);
    if (res < 0) return push_error(ls, fs, res);
    return 1;
}

static int Filesystem_opendir(lua_State* ls) {
    Filesystem* fs = check_mounted_Filesystem(ls, 1);
    char const* path = luaL_checkstring(ls, 2);

    Dir* d = lua_newuserdatauv(ls, sizeof(Dir), 1);
    luaL_getmetatable(ls, Dir_name);
    lua_setmetatable(ls, -2);
    lua_pushvalue(ls, 1);  // Keep fs alive
    lua_setiuservalue(ls, -2, 1);
    memset(d, 0, sizeof(Dir));

    int res = lfs_dir_open(&fs->lfs, &d->dir, path);
    if (res < 0) return push_error(ls, fs, res);
    return 1;
}

static int Filesystem_format(lua_State* ls) {
    Filesystem* fs = check_Filesystem(ls, 1);
    if (fs->mounted) return luaL_error(ls, "filesystem is mounted");
    return push_lfs_result_bool(ls, fs, lfs_format(&fs->lfs, &fs->config));
}

static int Filesystem_mkdir(lua_State* ls) {
    Filesystem* fs = check_mounted_Filesystem(ls, 1);
    char const* path = luaL_checkstring(ls, 2);
    return push_lfs_result_bool(ls, fs, lfs_mkdir(&fs->lfs, path));
}

MLUA_SYMBOLS(Filesystem_syms) = {
    MLUA_SYM_F(mount, Filesystem_),
    MLUA_SYM_F(unmount, Filesystem_),
    // MLUA_SYM_F(stat, Filesystem_),
    // MLUA_SYM_F(getattr, Filesystem_),
    MLUA_SYM_F(open, Filesystem_),
    MLUA_SYM_F(opendir, Filesystem_),
    // MLUA_SYM_F(statvfs, Filesystem_),
    // MLUA_SYM_F(size, Filesystem_),
    // MLUA_SYM_F(traverse, Filesystem_),
    // MLUA_SYM_F(gc, Filesystem_),
#ifndef LFS_READONLY
    MLUA_SYM_F(format, Filesystem_),
    // MLUA_SYM_F(remove, Filesystem_),
    // MLUA_SYM_F(rename, Filesystem_),
    // MLUA_SYM_F(setattr, Filesystem_),
    // MLUA_SYM_F(removeattr, Filesystem_),
    MLUA_SYM_F(mkdir, Filesystem_),
    // MLUA_SYM_F(mkconsistent, Filesystem_),
    // MLUA_SYM_F(grow, Filesystem_),
#else
    MLUA_SYM_V(format, boolean, false),
    // MLUA_SYM_V(remove, boolean, false),
    // MLUA_SYM_V(rename, boolean, false),
    // MLUA_SYM_V(setattr, boolean, false),
    // MLUA_SYM_V(removeattr, boolean, false),
    MLUA_SYM_V(mkdir, boolean, false),
    // MLUA_SYM_V(mkconsistent, boolean, false),
    // MLUA_SYM_V(grow, boolean, false),
#endif
#if !defined(LFS_READONLY) && defined(LFS_MIGRATE)
    // MLUA_SYM_F(migrate, Filesystem_),
#else
    // MLUA_SYM_V(migrate, boolean, false),
#endif
};

#define Filesystem___close Filesystem_unmount
#define Filesystem___gc Filesystem_unmount

MLUA_SYMBOLS_NOHASH(Filesystem_syms_nh) = {
    MLUA_SYM_F_NH(__close, Filesystem_),
    MLUA_SYM_F_NH(__gc, Filesystem_),
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
    lua_pop(ls, 1);
    lua_pushnil(ls);  // Mark as closed
    lua_setiuservalue(ls, 1, 1);
    return push_lfs_result_bool(ls, fs, lfs_file_close(&fs->lfs, &f->file));
}

static int File_sync(lua_State* ls) {
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    return push_lfs_result_bool(ls, fs, lfs_file_sync(&fs->lfs, &f->file));
}

static int File_read(lua_State* ls) {
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    lfs_size_t size = luaL_checkinteger(ls, 2);
    luaL_Buffer buf;
    void* dst = luaL_buffinitsize(ls, &buf, size);
    lfs_ssize_t res = lfs_file_read(&fs->lfs, &f->file, dst, size);
    if (res < 0) return push_error(ls, fs, res);
    return luaL_pushresultsize(&buf, res), 1;
}

static int File_seek(lua_State* ls) {
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    lfs_soff_t off = luaL_checkinteger(ls, 2);
    int whence = luaL_checkinteger(ls, 3);
    return push_lfs_result_int(ls, fs,
        lfs_file_seek(&fs->lfs, &f->file, off, whence));
}

static int File_tell(lua_State* ls) {
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    return push_lfs_result_int(ls, fs, lfs_file_tell(&fs->lfs, &f->file));
}

static int File_rewind(lua_State* ls) {
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    return push_lfs_result_int(ls, fs, lfs_file_rewind(&fs->lfs, &f->file));
}

static int File_size(lua_State* ls) {
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    return push_lfs_result_int(ls, fs, lfs_file_size(&fs->lfs, &f->file));
}

static int File_write(lua_State* ls) {
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    size_t size;
    void const* src = luaL_checklstring(ls, 2, &size);
    lfs_ssize_t res = lfs_file_write(&fs->lfs, &f->file, src, size);
    if (res < 0) return push_error(ls, fs, res);
    return lua_pushinteger(ls, res), 1;
}

static int File_truncate(lua_State* ls) {
    Filesystem* fs = NULL;
    File* f = check_File(ls, 1, &fs);
    lfs_off_t size = luaL_checkinteger(ls, 2);
    return push_lfs_result_bool(ls, fs,
        lfs_file_truncate(&fs->lfs, &f->file, size));
}

MLUA_SYMBOLS(File_syms) = {
    MLUA_SYM_F(close, File_),
    MLUA_SYM_F(sync, File_),
    MLUA_SYM_F(read, File_),
    MLUA_SYM_F(seek, File_),
    MLUA_SYM_F(tell, File_),
    MLUA_SYM_F(rewind, File_),
    MLUA_SYM_F(size, File_),
#ifndef LFS_READONLY
    MLUA_SYM_F(write, File_),
    MLUA_SYM_F(truncate, File_),
#else
    MLUA_SYM_V(write, boolean, false),
    MLUA_SYM_V(truncate, boolean, false),
#endif
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

static int Dir_close(lua_State* ls) {
    Dir* d = luaL_checkudata(ls, 1, Dir_name);
    if (lua_getiuservalue(ls, 1, 1) == LUA_TNIL) {  // Already closed
        return lua_pushboolean(ls, true), 1;
    }
    Filesystem* fs = to_Filesystem(ls, -1);
    lua_pop(ls, 1);
    lua_pushnil(ls);  // Mark as closed
    lua_setiuservalue(ls, 1, 1);
    return push_lfs_result_bool(ls, fs, lfs_dir_close(&fs->lfs, &d->dir));
}

static int Dir_read(lua_State* ls) {
    Filesystem* fs = NULL;
    Dir* d = check_Dir(ls, 1, &fs);
    struct lfs_info info;
    int res = lfs_dir_read(&fs->lfs, &d->dir, &info);
    if (res < 0) return push_error(ls, fs, res);
    if (res == 0) return lua_pushboolean(ls, true), 1;
    lua_pushstring(ls, info.name);
    lua_pushinteger(ls, info.type);
    lua_pushinteger(ls, info.size);
    return 3;
}

static int Dir_seek(lua_State* ls) {
    Filesystem* fs = NULL;
    Dir* d = check_Dir(ls, 1, &fs);
    lfs_soff_t off = luaL_checkinteger(ls, 2);
    return push_lfs_result_bool(ls, fs, lfs_dir_seek(&fs->lfs, &d->dir, off));
}

static int Dir_tell(lua_State* ls) {
    Filesystem* fs = NULL;
    Dir* d = check_Dir(ls, 1, &fs);
    return push_lfs_result_int(ls, fs, lfs_dir_tell(&fs->lfs, &d->dir));
}

static int Dir_rewind(lua_State* ls) {
    Filesystem* fs = NULL;
    Dir* d = check_Dir(ls, 1, &fs);
    return push_lfs_result_bool(ls, fs, lfs_dir_rewind(&fs->lfs, &d->dir));
}

MLUA_SYMBOLS(Dir_syms) = {
    MLUA_SYM_F(close, Dir_),
    MLUA_SYM_F(read, Dir_),
    MLUA_SYM_F(seek, Dir_),
    MLUA_SYM_F(tell, Dir_),
    MLUA_SYM_F(rewind, Dir_),
};

#define Dir___close Dir_close
#define Dir___gc Dir_close

MLUA_SYMBOLS_NOHASH(Dir_syms_nh) = {
    MLUA_SYM_F_NH(__close, Dir_),
    MLUA_SYM_F_NH(__gc, Dir_),
};

static int mod_Filesystem(lua_State* ls) {
    MLuaBlockDev* dev = mlua_check_BlockDev(ls, 1);
    Filesystem* fs = lua_newuserdatauv(
        ls, sizeof(Filesystem) + 2 * dev->write_size, 1);
    luaL_getmetatable(ls, Filesystem_name);
    lua_setmetatable(ls, -2);
    lua_pushvalue(ls, 1);  // Keep dev alive
    lua_setiuservalue(ls, -2, 1);
    fs->config = config_base;
    fs->config.context = dev;
    fs->config.read_size = dev->read_size;
    fs->config.prog_size = dev->write_size;
    fs->config.block_size = dev->erase_size;
    fs->config.block_count = dev->size / dev->erase_size;
    fs->config.cache_size = fs->config.prog_size;
    fs->config.read_buffer = fs->buffers;
    fs->config.prog_buffer = &fs->buffers[fs->config.cache_size];
    fs->config.lookahead_buffer = fs->lookahead_buffer;
    fs->mounted = false;
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(NAME_MAX, integer, LFS_NAME_MAX),
    MLUA_SYM_V(FILE_MAX, integer, LFS_FILE_MAX),
    MLUA_SYM_V(ATTR_MAX, integer, LFS_ATTR_MAX),
    MLUA_SYM_V(ERR_OK, integer, LFS_ERR_OK),
    MLUA_SYM_V(ERR_IO, integer, LFS_ERR_IO),
    MLUA_SYM_V(ERR_CORRUPT, integer, LFS_ERR_CORRUPT),
    MLUA_SYM_V(ERR_NOENT, integer, LFS_ERR_NOENT),
    MLUA_SYM_V(ERR_EXIST, integer, LFS_ERR_EXIST),
    MLUA_SYM_V(ERR_NOTDIR, integer, LFS_ERR_NOTDIR),
    MLUA_SYM_V(ERR_ISDIR, integer, LFS_ERR_ISDIR),
    MLUA_SYM_V(ERR_NOTEMPTY, integer, LFS_ERR_NOTEMPTY),
    MLUA_SYM_V(ERR_BADF, integer, LFS_ERR_BADF),
    MLUA_SYM_V(ERR_FBIG, integer, LFS_ERR_FBIG),
    MLUA_SYM_V(ERR_INVAL, integer, LFS_ERR_INVAL),
    MLUA_SYM_V(ERR_NOSPC, integer, LFS_ERR_NOSPC),
    MLUA_SYM_V(ERR_NOMEM, integer, LFS_ERR_NOMEM),
    MLUA_SYM_V(ERR_NOATTR, integer, LFS_ERR_NOATTR),
    MLUA_SYM_V(ERR_NAMETOOLONG, integer, LFS_ERR_NAMETOOLONG),
    MLUA_SYM_V(TYPE_REG, integer, LFS_TYPE_REG),
    MLUA_SYM_V(TYPE_DIR, integer, LFS_TYPE_DIR),
    MLUA_SYM_V(O_RDONLY, integer, LFS_O_RDONLY),
#ifndef LFS_READONLY
    MLUA_SYM_V(O_WRONLY, integer, LFS_O_WRONLY),
    MLUA_SYM_V(O_RDWR, integer, LFS_O_RDWR),
    MLUA_SYM_V(O_CREAT, integer, LFS_O_CREAT),
    MLUA_SYM_V(O_EXCL, integer, LFS_O_EXCL),
    MLUA_SYM_V(O_TRUNC, integer, LFS_O_TRUNC),
    MLUA_SYM_V(O_APPEND, integer, LFS_O_APPEND),
#else
    MLUA_SYM_V(O_WRONLY, boolean, false),
    MLUA_SYM_V(O_RDWR, boolean, false),
    MLUA_SYM_V(O_CREAT, boolean, false),
    MLUA_SYM_V(O_EXCL, boolean, false),
    MLUA_SYM_V(O_TRUNC, boolean, false),
    MLUA_SYM_V(O_APPEND, boolean, false),
#endif
    MLUA_SYM_V(SEEK_SET, integer, LFS_SEEK_SET),
    MLUA_SYM_V(SEEK_CUR, integer, LFS_SEEK_CUR),
    MLUA_SYM_V(SEEK_END, integer, LFS_SEEK_END),

    MLUA_SYM_F(Filesystem, mod_),
};

MLUA_OPEN_MODULE(mlua.fs.lfs) {
    mlua_require(ls, "mlua.block", false);

    // Create the Filesystem class.
    mlua_new_class(ls, Filesystem_name, Filesystem_syms, true);
    mlua_set_fields(ls, Filesystem_syms_nh);
    lua_pop(ls, 1);

    // Create the File class.
    mlua_new_class(ls, File_name, File_syms, true);
    mlua_set_fields(ls, File_syms_nh);
    lua_pop(ls, 1);

    // Create the Dir class.
    mlua_new_class(ls, Dir_name, Dir_syms, true);
    mlua_set_fields(ls, Dir_syms_nh);
    lua_pop(ls, 1);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);
    return 1;
}