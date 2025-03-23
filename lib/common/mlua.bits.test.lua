-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local bits = require 'mlua.bits'
local int64 = require 'mlua.int64'
local string = require 'string'
local table = require 'table'

local function hex(v)
    if type(v) == 'number' then return ('0x%x'):format(v) end
    return '0x' .. int64.hex(v)
end

function test_bit_counts(t)
    local ilz = string.packsize('j') * 8 - 32
    for _, test in ipairs{
        {0x00000000, ilz + 32, ilz + 32, 0},
        {0x00000001, ilz + 31, 0, 1},
        {0x00000002, ilz + 30, 1, 1},
        {0x0000000c, ilz + 28, 2, 2},
        {0x023d6800, ilz + 6, 11, 9},
        {0x6a7e0000, ilz + 1, 17, 10},
        {0x80000000, ilz + 0, 31, 1},
        {int64('0x0000000000000001'), 63, 0, 1},
        {int64('0x0000056789ab0000'), 21, 16, 15},
        {int64('0x8000000000000000'), 0, 63, 1},
        {int64('0x0000000000000000'), 64, 64, 0},
    } do
        local arg, lz, tz, ones = table.unpack(test)
        t:expect(t.expr(bits).leading_zeros(arg)):eq(lz)
        t:expect(t.expr(bits).trailing_zeros(arg)):eq(tz)
        t:expect(t.expr(bits).ones(arg)):eq(ones)
        t:expect(t.expr(bits).parity(arg)):eq(ones % 2)
    end
end

function test_mask(t)
    for _, test in ipairs{
        {0, 0x00000000},
        {1, 0x00000001},
        {5, 0x0000001f},
        {19, 0x0007ffff},
        {31, 0x7fffffff},
        {32, 0xffffffff},
        {33, int64('0x00000001ffffffff')},
        {54, int64('0x003fffffffffffff')},
        {63, int64('0x7fffffffffffffff')},
        {64, int64('0xffffffffffffffff')},
    } do
        local arg, want = table.unpack(test)
        t:expect(t.expr(bits).mask(arg)):fmt(hex):eq(want)
    end
end
