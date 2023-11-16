-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = mlua.Module(...)

local flash = require 'mlua.block.flash'
local lfs = require 'mlua.fs.lfs'
local list = require 'mlua.list'
local util = require 'mlua.util'
local pico = require 'pico'
local table = require 'table'

local fs

function set_up(t)
    -- Create a filesystem in the unused part of the flash and mount it.
    local off = pico.flash_binary_end - pico.flash_binary_start
    fs = lfs.Filesystem(flash.Device(off, pico.FLASH_SIZE_BYTES - off))
    t:assert(fs:format())
    t:assert(fs:mount())
    t:cleanup(function() fs:unmount() end)
end

local function write_file(fs, path, data)
    local f<close> = assert(fs:open(path, lfs.O_WRONLY | lfs.O_CREAT))
    f:write(data)
end

function test_file(t)
    do
        local f<close> = assert(fs:open('/test', lfs.O_WRONLY | lfs.O_CREAT))
        t:expect(t:expr(f):write("The quick brown fox")):eq(19)
        t:expect(t:expr(f):tell()):eq(19)
        t:expect(t:expr(f):sync()):eq(true)
        t:expect(t:expr(f):write(" jumps over the lazy dog")):eq(24)
        t:expect(t:expr(f):size()):eq(43)
    end
    do
        local f<close> = assert(fs:open('/test', lfs.O_RDWR))
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
        local f<close> = assert(fs:open('/test', lfs.O_RDONLY))
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
    fs:mkdir('/dir')
    fs:mkdir('/dir/sub')
    write_file(fs, '/dir/file1', '12345')
    write_file(fs, '/dir/file2', '12345678')
    write_file(fs, '/dir/sub/file', '123')

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
        {'.', lfs.TYPE_DIR, 0},
        {'..', lfs.TYPE_DIR, 0},
        {'file1', lfs.TYPE_REG, 5},
        {'file2', lfs.TYPE_REG, 8},
        {'sub', lfs.TYPE_DIR, 0},
    }
    t:expect(t.expr.read_dir(fs, '/dir/sub')):eq{
        {'.', lfs.TYPE_DIR, 0},
        {'..', lfs.TYPE_DIR, 0},
        {'file', lfs.TYPE_REG, 3},
    }
end

