-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local fs = require 'mlua.fs'
local loader = require 'mlua.fs.loader'
local util = require 'mlua.util'
local package = require 'package'
local multicore = require 'pico.multicore'
local fifo = require 'pico.multicore.fifo'
local string = require 'string'

local prefix = ...

function set_up(t)
    t:expect(loader.block, "no block device");
    t:assert(loader.fs, "no filesystem");

    if loader.fs:is_mounted() then loader.fs:unmount() end
    t:assert(t.expr(loader.fs):format()):eq(true)
    t:assert(t.expr(loader.fs):mount()):eq(true)
    assert(loader.fs:mkdir('/lua'))
end

function test_block_device(t)
    t:expect(t.expr(loader).block:size()):eq(100 << 10)
end

local function write_file(mfs, path, data)
    local f<close> = assert(
        mfs:open(path, fs.O_WRONLY | fs.O_CREAT | fs.O_TRUNC))
    f:write(data)
end

function test_loading(t)
    local path = package.path
    local mod_a, mod_b = prefix .. '.a', prefix .. '.b'
    t:cleanup(function()
        package.loaded[mod_a] = nil
        package.loaded[mod_b] = nil
        package.path = path
    end)
    package.path = '/lua/?.lua;/?.lua'

    local path_a = ('/lua/%s.a.lua'):format(prefix)
    write_file(loader.fs, path_a, [[
        args = {...}
        value = 'a'
    ]])
    local path_b = ('/%s.b.lua'):format(prefix)
    write_file(loader.fs, path_b, [[
        args = {...}
        value = 'b'
    ]])

    local m, p = require(mod_a)
    t:expect(t.expr(m).args):eq({mod_a, path_a}, util.table_eq)
    t:expect(t.expr(m).value):eq('a')
    t:expect(p):label("a.path"):eq(path_a)

    local m, p = require(mod_b)
    t:expect(t.expr(m).args):eq({mod_b, path_b}, util.table_eq)
    t:expect(t.expr(m).value):eq('b')
    t:expect(p):label("b.path"):eq(path_b)

    t:expect(t.expr(_G).require('not_found'))
        :raises("\t/lua/not_found%.lua: no such file or directory\n\t/not_found%.lua: no such file or directory")

    write_file(loader.fs, '/invalid.lua', 'abcde')
    t:expect(t.expr(_G).require('invalid'))
        :raises("\t/invalid%.lua:1: syntax error")
end

function test_loading_multicore(t)
    write_file(loader.fs, ('/lua/%s.c.lua'):format(prefix), [[
        local multicore = require 'pico.multicore'
        local fifo = require 'pico.multicore.fifo'

        function core1_echo()
            multicore.set_shutdown_handler()
            fifo.enable_irq()
            local done<close> = function() fifo.enable_irq(false) end
            while true do
                fifo.push_blocking(fifo.pop_blocking())
            end
        end
    ]])

    multicore.launch_core1(prefix .. '.c', 'core1_echo')
    t:cleanup(multicore.reset_core1)
    fifo.enable_irq()
    t:cleanup(function() fifo.enable_irq(false) end)
    for i = 1, 10 do
        fifo.push_blocking(i)
        t:expect(t.expr(fifo).pop_blocking()):eq(i)
    end
end
