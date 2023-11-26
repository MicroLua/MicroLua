-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = mlua.module(...)

local flash = require 'mlua.block.flash'
local errors = require 'mlua.errors'
local fs = require 'mlua.fs'
local lfs = require 'mlua.fs.lfs'
local list = require 'mlua.list'
local util = require 'mlua.util'
local pico = require 'pico'
local table = require 'table'

local dev, dfs

local function write_file(path, data)
    local f<close> = assert(dfs:open(path, fs.O_WRONLY | fs.O_CREAT))
    f:write(data)
end

function set_up(t)
    -- Create a filesystem in the unused part of the flash and mount it.
    local off = pico.flash_binary_end - pico.flash_binary_start
    dev = flash.new(off, pico.FLASH_SIZE_BYTES - off)
    local size = dev:size()
    dfs = lfs.new(dev)
    t:assert(dfs:format(size // 2))  -- Half-size to allow growing
    t:expect(t:expr(dfs):is_mounted()):eq(false)
    t:assert(dfs:mount())
    t:expect(t:expr(dfs):is_mounted()):eq(true)
    t:cleanup(function() dfs:unmount() end)
    dfs:mkdir('/dir')
    dfs:mkdir('/dir/sub')
    write_file('/dir/file1', '12345')
    write_file('/dir/file2', '12345678')
    write_file('/dir/sub/file', '123')
end

function test_stat(t)
    t:expect(t:mexpr(dfs):stat('/')):eq{'/', fs.TYPE_DIR}
    t:expect(t:mexpr(dfs):stat('/dir')):eq{'dir', fs.TYPE_DIR}
    t:expect(t:mexpr(dfs):stat('/dir/file1')):eq{'file1', fs.TYPE_REG, 5}
    t:expect(t:mexpr(dfs):stat('/dir/file2')):eq{'file2', fs.TYPE_REG, 8}
    t:expect(t:mexpr(dfs):stat('/not-exists'))
        :eq{nil, "no such file or directory", errors.ENOENT}
end

function test_attrs(t)
    local attr, value = 0x42, '0123456789012345678901234567890123456789'
    t:expect(t:expr(dfs):getattr('/dir/file1', attr)):eq(nil)
    t:expect(t:expr(dfs):setattr('/dir/file1', attr, value)):eq(true)
    t:expect(t:expr(dfs):getattr('/dir/file1', attr)):eq(value)
    t:expect(t:expr(dfs):removeattr('/dir/file1', attr)):eq(true)
    t:expect(t:expr(dfs):getattr('/dir/file1', attr)):eq(nil)
end

function test_statvfs(t)
    local dv, bs, bc, name_max, file_max, attr_max = dfs:statvfs()
    t:expect(dv):label("disk_version"):eq(lfs.DISK_VERSION)
    local size, _, _, erase_size = dev:size()
    t:expect(bs):label("block_size"):eq(erase_size)
    t:expect(bc):label("block_count"):eq(size // erase_size // 2)
    t:expect(name_max):label("name_max"):eq(lfs.NAME_MAX)
    t:expect(file_max):label("file_max"):eq(lfs.FILE_MAX)
    t:expect(attr_max):label("attr_max"):eq(lfs.ATTR_MAX)

    -- Grow the filesystem to the full size of the device.
    t:expect(t:expr(dfs):mkconsistent()):eq(true)
    t:expect(t:expr(dfs):grow()):eq(true)
    local _, _, bc2 = dfs:statvfs()
    t:expect(bc2):label("block_count"):eq(size // erase_size)
end

function test_block_accounting(t)
    t:expect(t:expr(dfs):gc()):eq(true)
    t:expect(t:expr(dfs):size()):gt(0)
    local bc = 0
    t:expect(t:expr(dfs):traverse(function(block) bc = bc + 1 end)):eq(true)
    -- size() is implemented in terms of traverse(), but the latter includes
    -- some blocks that the former doesn't. Hence the ">=".
    t:expect(bc):label("block count"):gte(dfs:size())
end

function test_remove(t)
    dfs:mkdir('/remove')
    write_file('/remove/file', '123')
    t:expect(t:expr(dfs):stat('/remove')):neq(nil)
    t:expect(t:expr(dfs):stat('/remove/file')):neq(nil)
    t:expect(t:expr(dfs):remove('/remove')):eq(nil)  -- Directory not empty
    t:expect(t:expr(dfs):remove('/remove/file')):eq(true)
    t:expect(t:expr(dfs):stat('/remove/file')):eq(nil)
    t:expect(t:expr(dfs):remove('/remove')):eq(true)
    t:expect(t:expr(dfs):stat('/remove')):eq(nil)
end

function test_rename(t)
    dfs:mkdir('/rename')
    write_file('/rename/file', '123')
    t:expect(t:expr(dfs):rename('/rename/file', '/file-new')):eq(true)
    t:expect(t:expr(dfs):stat('/rename/file')):eq(nil)
    t:expect(t:expr(dfs):stat('/file-new')):neq(nil)
    t:expect(t:expr(dfs):rename('/rename', '/dir-new')):eq(true)
    t:expect(t:expr(dfs):stat('/rename')):eq(nil)
    t:expect(t:expr(dfs):stat('/dir-new')):neq(nil)
end

function test_migrate(t)
    if not dfs.migrate then t:skip("LFS_MIGRATE undefined") end
    -- Testing this requires an existing LFS1 filesystem, which I don't happen
    -- to have at hand. So I'm going to trust visual inspection for now.
end

function test_file(t)
    do
        local f<close> = assert(dfs:open('/file', fs.O_WRONLY | fs.O_CREAT))
        t:expect(t:expr(f):write("The quick brown fox")):eq(19)
        t:expect(t:expr(f):tell()):eq(19)
        t:expect(t:expr(f):sync()):eq(true)
        t:expect(t:expr(f):write(" jumps over the lazy dog")):eq(24)
        t:expect(t:expr(f):size()):eq(43)
    end
    do
        local f<close> = assert(dfs:open('/file', fs.O_RDWR))
        t:expect(t:expr(f):size()):eq(43)
        t:expect(t:expr(f):read(9)):eq("The quick")
        t:expect(t:expr(f):tell()):eq(9)
        t:expect(t:expr(f):seek(16)):eq(16)
        t:expect(t:expr(f):read(3)):eq("fox")
        t:expect(t:expr(f):tell()):eq(19)
        t:expect(t:expr(f):seek(7, fs.SEEK_CUR)):eq(26)
        t:expect(t:expr(f):read(4)):eq("over")
        t:expect(t:expr(f):seek(-3, fs.SEEK_END)):eq(40)
        t:expect(t:expr(f):read(10)):eq("dog")

        t:expect(t:expr(f):rewind()):eq(0)
        t:expect(t:expr(f):read(20)):eq("The quick brown fox ")
        t:expect(t:expr(f):write("flies")):eq(5)
        t:expect(t:expr(f):truncate(35)):eq(true)
        t:expect(t:expr(f):size()):eq(35)
        t:expect(t:expr(f):seek(0, fs.SEEK_END)):eq(35)
        t:expect(t:expr(f):write("fence")):eq(5)
    end
    do
        local f<close> = assert(dfs:open('/file', fs.O_RDONLY))
        t:expect(t:expr(f):read(100))
            :eq("The quick brown fox flies over the fence")
    end
end

local function read_dir(path)
    local d<close> = assert(dfs:opendir(path))
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
        local d<close> = assert(dfs:opendir('/dir'))
        t:expect(t:expr(d):read()):eq('.')
        local pos = assert(d:tell())
        t:expect(t:expr(d):read()):eq('..')
        t:expect(t:expr(d):seek(pos)):eq(true)
        t:expect(t:expr(d):read()):eq('..')
        t:expect(t:expr(d):rewind()):eq(true)
        t:expect(t:expr(d):read()):eq('.')
    end
    t:expect(t.expr.read_dir('/dir')):eq{
        {'.', fs.TYPE_DIR},
        {'..', fs.TYPE_DIR},
        {'file1', fs.TYPE_REG, 5},
        {'file2', fs.TYPE_REG, 8},
        {'sub', fs.TYPE_DIR},
    }
    t:expect(t.expr.read_dir('/dir/sub')):eq{
        {'.', fs.TYPE_DIR},
        {'..', fs.TYPE_DIR},
        {'file', fs.TYPE_REG, 3},
    }
end
