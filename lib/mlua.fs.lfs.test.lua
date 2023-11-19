-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = mlua.Module(...)

local flash = require 'mlua.block.flash'
local errors = require 'mlua.errors'
local lfs = require 'mlua.fs.lfs'
local list = require 'mlua.list'
local util = require 'mlua.util'
local pico = require 'pico'
local table = require 'table'

local dev, fs

local function write_file(fs, path, data)
    local f<close> = assert(fs:open(path, lfs.O_WRONLY | lfs.O_CREAT))
    f:write(data)
end

function set_up(t)
    -- Create a filesystem in the unused part of the flash and mount it.
    local off = pico.flash_binary_end - pico.flash_binary_start
    dev = flash.new(off, pico.FLASH_SIZE_BYTES - off)
    local size = dev:size()
    fs = lfs.Filesystem(dev)
    t:assert(fs:format(size // 2))  -- Half-size to allow growing
    t:assert(fs:mount())
    t:cleanup(function() fs:unmount() end)
    fs:mkdir('/dir')
    fs:mkdir('/dir/sub')
    write_file(fs, '/dir/file1', '12345')
    write_file(fs, '/dir/file2', '12345678')
    write_file(fs, '/dir/sub/file', '123')
end

function test_stat(t)
    t:expect(t:mexpr(fs):stat('/')):eq{'/', lfs.TYPE_DIR}
    t:expect(t:mexpr(fs):stat('/dir')):eq{'dir', lfs.TYPE_DIR}
    t:expect(t:mexpr(fs):stat('/dir/file1')):eq{'file1', lfs.TYPE_REG, 5}
    t:expect(t:mexpr(fs):stat('/dir/file2')):eq{'file2', lfs.TYPE_REG, 8}
    t:expect(t:mexpr(fs):stat('/not-exists'))
        :eq{nil, "no such file or directory", errors.ENOENT}
end

function test_attrs(t)
    local attr, value = 0x42, '0123456789012345678901234567890123456789'
    t:expect(t:expr(fs):getattr('/dir/file1', attr)):eq(nil)
    t:expect(t:expr(fs):setattr('/dir/file1', attr, value)):eq(true)
    t:expect(t:expr(fs):getattr('/dir/file1', attr)):eq(value)
    t:expect(t:expr(fs):removeattr('/dir/file1', attr)):eq(true)
    t:expect(t:expr(fs):getattr('/dir/file1', attr)):eq(nil)
end

function test_statvfs(t)
    local dv, bs, bc, name_max, file_max, attr_max = fs:statvfs()
    t:expect(dv):label("disk_version"):eq(lfs.DISK_VERSION)
    local size, _, _, erase_size = dev:size()
    t:expect(bs):label("block_size"):eq(erase_size)
    t:expect(bc):label("block_count"):eq(size // erase_size // 2)
    t:expect(name_max):label("name_max"):eq(lfs.NAME_MAX)
    t:expect(file_max):label("file_max"):eq(lfs.FILE_MAX)
    t:expect(attr_max):label("attr_max"):eq(lfs.ATTR_MAX)

    -- Grow the filesystem to the full size of the device.
    t:expect(t:expr(fs):mkconsistent()):eq(true)
    t:expect(t:expr(fs):grow()):eq(true)
    local _, _, bc2 = fs:statvfs()
    t:expect(bc2):label("block_count"):eq(size // erase_size)
end

function test_block_accounting(t)
    t:expect(t:expr(fs):gc()):eq(true)
    t:expect(t:expr(fs):size()):gt(0)
    local bc = 0
    t:expect(t:expr(fs):traverse(function(block) bc = bc + 1 end)):eq(true)
    -- size() is implemented in terms of traverse(), but the latter includes
    -- some blocks that the former doesn't. Hence the ">=".
    t:expect(bc):label("block count"):gte(fs:size())
end

function test_remove(t)
    fs:mkdir('/remove')
    write_file(fs, '/remove/file', '123')
    t:expect(t:expr(fs):stat('/remove')):neq(nil)
    t:expect(t:expr(fs):stat('/remove/file')):neq(nil)
    t:expect(t:expr(fs):remove('/remove')):eq(nil)  -- Directory not empty
    t:expect(t:expr(fs):remove('/remove/file')):eq(true)
    t:expect(t:expr(fs):stat('/remove/file')):eq(nil)
    t:expect(t:expr(fs):remove('/remove')):eq(true)
    t:expect(t:expr(fs):stat('/remove')):eq(nil)
end

function test_rename(t)
    fs:mkdir('/rename')
    write_file(fs, '/rename/file', '123')
    t:expect(t:expr(fs):rename('/rename/file', '/file-new')):eq(true)
    t:expect(t:expr(fs):stat('/rename/file')):eq(nil)
    t:expect(t:expr(fs):stat('/file-new')):neq(nil)
    t:expect(t:expr(fs):rename('/rename', '/dir-new')):eq(true)
    t:expect(t:expr(fs):stat('/rename')):eq(nil)
    t:expect(t:expr(fs):stat('/dir-new')):neq(nil)
end

function test_migrate(t)
    if not fs.migrate then t:skip("LFS_MIGRATE undefined") end
    -- Testing this requires an existing LFS1 filesystem, which I don't happen
    -- to have at hand. So I'm going to trust visual inspection for now.
end

function test_file(t)
    do
        local f<close> = assert(fs:open('/file', lfs.O_WRONLY | lfs.O_CREAT))
        t:expect(t:expr(f):write("The quick brown fox")):eq(19)
        t:expect(t:expr(f):tell()):eq(19)
        t:expect(t:expr(f):sync()):eq(true)
        t:expect(t:expr(f):write(" jumps over the lazy dog")):eq(24)
        t:expect(t:expr(f):size()):eq(43)
    end
    do
        local f<close> = assert(fs:open('/file', lfs.O_RDWR))
        t:expect(t:expr(f):size()):eq(43)
        t:expect(t:expr(f):read(9)):eq("The quick")
        t:expect(t:expr(f):tell()):eq(9)
        t:expect(t:expr(f):seek(16, lfs.SEEK_SET)):eq(16)
        t:expect(t:expr(f):read(3)):eq("fox")
        t:expect(t:expr(f):tell()):eq(19)
        t:expect(t:expr(f):seek(7, lfs.SEEK_CUR)):eq(26)
        t:expect(t:expr(f):read(4)):eq("over")
        t:expect(t:expr(f):seek(-3, lfs.SEEK_END)):eq(40)
        t:expect(t:expr(f):read(10)):eq("dog")

        t:expect(t:expr(f):rewind()):eq(0)
        t:expect(t:expr(f):read(20)):eq("The quick brown fox ")
        t:expect(t:expr(f):write("flies")):eq(5)
        t:expect(t:expr(f):truncate(35)):eq(true)
        t:expect(t:expr(f):size()):eq(35)
        t:expect(t:expr(f):seek(0, lfs.SEEK_END)):eq(35)
        t:expect(t:expr(f):write("fence")):eq(5)
    end
    do
        local f<close> = assert(fs:open('/file', lfs.O_RDONLY))
        t:expect(t:expr(f):read(100))
            :eq("The quick brown fox flies over the fence")
    end
end

local function read_dir(fs, path)
    local d<close> = assert(fs:opendir(path))
    local entries = list()
    while true do
        local name, type, size = assert(d:read())
        if name == true then break end
        entries:append(list{name, type, size})
    end
    table.sort(entries, util.table_comp{1})
    return entries
end

function test_dir(t)
    local _ = read_dir  -- Capture the upvalue
    do
        local d<close> = assert(fs:opendir('/dir'))
        t:expect(t:expr(d):read()):eq('.')
        local pos = assert(d:tell())
        t:expect(t:expr(d):read()):eq('..')
        t:expect(t:expr(d):seek(pos)):eq(true)
        t:expect(t:expr(d):read()):eq('..')
        t:expect(t:expr(d):rewind()):eq(true)
        t:expect(t:expr(d):read()):eq('.')
    end
    t:expect(t.expr.read_dir(fs, '/dir')):eq{
        {'.', lfs.TYPE_DIR},
        {'..', lfs.TYPE_DIR},
        {'file1', lfs.TYPE_REG, 5},
        {'file2', lfs.TYPE_REG, 8},
        {'sub', lfs.TYPE_DIR},
    }
    t:expect(t.expr.read_dir(fs, '/dir/sub')):eq{
        {'.', lfs.TYPE_DIR},
        {'..', lfs.TYPE_DIR},
        {'file', lfs.TYPE_REG, 3},
    }
end
