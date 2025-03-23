-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local list = require 'mlua.list'
local mem = require 'mlua.mem'
local bootrom = require 'pico.bootrom'

local function read_str(code)
    return mem.read_cstr(bootrom.data_lookup(bootrom.table_code(code)))
end

function set_up(t)
    t:printf("Copyright: %s\n", read_str('CR'))
    local rev = read_str('GR')
    local parts = list()
    for i = 1, #rev do parts:append(('%02x'):format(rev:byte(i))) end
    t:printf("Version: %d, Git revision: %s\n", bootrom.VERSION, parts:concat())
end

function test_constants(t)
    t:expect(t.expr(bootrom).START):eq(pointer(0x00000000))
    t:expect(t.expr(bootrom).SIZE):eq(0x4000)
end

function test_table_code(t)
    t:expect(t.expr(bootrom).table_code('P3')):eq(bootrom.FUNC_POPCOUNT32)
    t:expect(t.expr(bootrom).table_code('MS')):eq(bootrom.FUNC_MEMSET)
end

function test_func_lookup(t)
    t:expect(t.expr(bootrom).func_lookup(0)):eq(pointer(0))
    t:expect(t.expr(bootrom).func_lookup(bootrom.FUNC_MEMSET)):neq(pointer(0))
end

function test_data_lookup(t)
    t:expect(t.expr(bootrom).data_lookup(0)):eq(pointer(0))
    t:expect(t.expr(bootrom).data_lookup(bootrom.table_code('CR')))
        :neq(pointer(0))
    t:expect(read_str('CR')):label("copyright")
        :matches('^%(C%) %d%d%d%d Raspberry Pi')
end
