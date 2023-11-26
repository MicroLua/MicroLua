-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = mlua.module(...)

local fs = require 'mlua.fs'
local loader = require 'mlua.fs.loader'
local package = require 'package'
local string = require 'string'

function set_up(t)
    -- Re-format the filesystem to avoid flakes due to a bad filesystem.
    t:assert(t:expr(loader.fs):is_mounted()):eq(true)
    t:assert(t:expr(loader.fs):unmount()):eq(true)
    t:assert(t:expr(loader.fs):format()):eq(true)
    t:assert(t:expr(loader.fs):mount()):eq(true)
end

function test_block_device(t)
    t:expect(t:expr(loader).block:size()):eq(100 << 10)
end

local function write_file(mfs, path, data)
    local f<close> = assert(
        mfs:open(path, fs.O_WRONLY | fs.O_CREAT | fs.O_TRUNC))
    f:write(data)
end

function test_loading(t)
    local path = package.path
    t:cleanup(function()
        package.loaded['mlua.a'] = nil
        package.loaded['mlua.b'] = nil
        package.path = path
    end)
    package.path = '/lua/?.lua;/?.lua'

    assert(loader.fs:mkdir('/lua'))
    write_file(loader.fs, '/lua/mlua.a.lua', [[
        _ENV = mlua.module(...)
        value = 'a'
    ]])
    write_file(loader.fs, '/mlua.b.lua', [[
        _ENV = mlua.module(...)
        value = 'b'
    ]])

    local m, p = require 'mlua.a'
    t:expect(t:expr(m).value):eq('a')
    t:expect(p):label("a.path"):eq('/lua/mlua.a.lua')

    local m, p = require 'mlua.b'
    t:expect(t:expr(m).value):eq('b')
    t:expect(p):label("b.path"):eq('/mlua.b.lua')

    t:expect(t:expr(_G).require('not_found'))
        :raises("\t/lua/not_found%.lua: no such file or directory\n" ..
                "\t/not_found%.lua: no such file or directory")

    write_file(loader.fs, '/invalid.lua', 'abcde')
    t:expect(t:expr(_G).require('invalid'))
        :raises("\t/invalid%.lua:1: syntax error")
end
