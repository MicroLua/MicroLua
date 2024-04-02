-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local math = require 'math'
local array = require 'mlua.array'
local int64 = require 'mlua.int64'
local list = require 'mlua.list'
local string = require 'string'
local table = require 'table'

function test_size(t)
    for _, test in ipairs{
        'b', 'B', 'h', 'H', 'i', 'I', 'l', 'L', 'j', 'J', 'T',
        'i1', 'i2', 'i3', 'i4', 'i5', 'i6', 'i7', 'i8',
        'I1', 'I2', 'I3', 'I4', 'I5', 'I6', 'I7', 'I8',
    } do
        t:expect(t:expr(array(test, 1)):size()):eq(string.packsize(test))
    end
    t:expect(t:expr(array)('i0')):raises("out of limits")
    t:expect(t:expr(array)('i9')):raises("out of limits")
    t:expect(t:expr(array)('x')):raises("invalid value format")
end

local function clamp(typ, value)
    local size = string.packsize(typ)
    if typ < 'a' then return value & ((1 << 8 * size) - 1) end
    local mask = (1 << 8 * size) - 1
    value = value & mask
    if math.ult(mask >> 1, value) then value = value - mask - 1 end
    return value
end

function test_integer(t)
    local values = {
        0, 1, -1,
        (1 << 7) + 2, -((1 << 7) + 2),
        (1 << 8) + 3, -((1 << 8) + 3),
        (1 << 15) + 4, -((1 << 15) + 4),
        (1 << 16) + 5, -((1 << 16) + 5),
        (1 << 31) + 6, -((1 << 31) + 6),
        (1 << 32) + 7, -((1 << 32) + 7),
        math.maxinteger, math.mininteger,
    }
    for _, test in ipairs{
        'b', 'B', 'h', 'H', 'i', 'I', 'l', 'L', 'j', 'J', 'T',
        'i1', 'i2', 'i3', 'i4', 'I1', 'I2', 'I3', 'I4',
    } do
        local a = array(test, #values):set(1, table.unpack(values))
        local want = {}
        for i, v in ipairs(values) do want[i] = clamp(test, v) end
        t:expect(t:mexpr(a):get(1, #a)):eq(want)
    end
end

local one64 = int64(1)

local function clamp64(typ, value)
    local size = string.packsize(typ)
    if typ < 'a' then return value & ((one64 << 8 * size) - 1) end
    local mask = (one64 << 8 * size) - 1
    value = value & mask
    if int64.ult(mask >> 1, value) then value = value - mask - 1 end
    return value
end

function _test_int64(t)
    local values = {
        0, one64, -one64,
        (one64 << 7) + 2, -((one64 << 7) + 2),
        (one64 << 8) + 3, -((one64 << 8) + 3),
        (one64 << 15) + 4, -((one64 << 15) + 4),
        (one64 << 16) + 5, -((one64 << 16) + 5),
        (one64 << 31) + 6, -((one64 << 31) + 6),
        (one64 << 32) + 7, -((one64 << 32) + 7),
        (one64 << 39) + 8, -((one64 << 39) + 8),
        (one64 << 40) + 9, -((one64 << 40) + 9),
        (one64 << 47) + 10, -((one64 << 47) + 10),
        (one64 << 48) + 11, -((one64 << 48) + 11),
        (one64 << 55) + 12, -((one64 << 55) + 12),
        (one64 << 56) + 13, -((one64 << 56) + 13),
        int64.max, int64.min,
    }
    for _, test in ipairs{
        'i5', 'i6', 'i7', 'i8', 'I5', 'I6', 'I7', 'I8',
    } do
        local a = array(test, #values):set(1, table.unpack(values))
        local want = {}
        for i, v in ipairs(values) do want[i] = clamp64(test, v) end
        t:expect(t:mexpr(a):get(1, #a)):eq(want)
    end
end

function test_eq(t)
    for _, test in ipairs{
        {array('j', 0), array('j', 0, 1):set(1, 1), true},
        {array('j', 0), array('j', 1), false},
        {array('j', 2):set(1, 1, 2), array('j', 2, 3):set(1, 1, 2, 3), true},
        {array('j', 2):set(1, 3, 4), array('j', 2, 3):set(1, 3, 99, 5), false},
        {array('j', 2):set(1, 5, 6), array('J', 2):set(1, 5, 6), true},
        {array('j', 2):set(1, 7, 8), array('J', 2):set(1, 99, 8), false},
        {array('b', 2):set(1, 9, -10), array('h', 2):set(1, 9, -10), true},
        {array('b', 2):set(1, 11, -12), array('h', 2):set(1, 11, -99), false},
        {array('i8', 2):set(1, 13, 14), array('i8', 2):set(1, 13, 14), true},
        {array('i8', 2):set(1, 15, 16), array('i8', 2):set(1, 16, 99), false},
        {array('i8', 2):set(1, 17, 18), array('i', 2):set(1, 17, 18), true},
        {array('i8', 2):set(1, 19, 20), array('i', 2):set(1, 19, 99), false},
    } do
        local a1, a2, want = table.unpack(test)
        t:expect(t:expr(_G).equal(a1, a2)):eq(want)
        t:expect(a1 == a2):label("%s == %s", a1, a2):eq(want)
    end
end

function test_tostring(t)
    for _, test in ipairs{
        {array('j', 0), '{}'},
        {array('j', 1):set(1, 1), '{1}'},
        {array('j', 3, 4):set(1, 1, -2, 3, 4), '{1, -2, 3}'},
    } do
        local value, want = table.unpack(test)
        t:expect(t.expr.tostring(value)):eq(want)
    end
end

function test_get(t)
    local a = array('j', 2, 4):set(1, 1, 2, 3, 4)
    t:expect(t:expr(a):len(0)):eq(2)
    t:expect(t:expr(a):len(3)):eq(0)
    t:expect(t:expr(a):len(-1)):raises("invalid length")
    t:expect(t:expr(a):len(5)):raises("invalid length")
    t:expect(t:expr(a):len()):eq(3)
    t:expect(#a):label("#a"):eq(3)
    t:expect(t:expr(a):cap()):eq(4)
    for _, test in ipairs{
        {{0}, list.pack(nil)}, {{1}, {1}}, {{2}, {2}}, {{3}, {3}},
        {{4}, list.pack(nil)}, {{-1}, {3}}, {{-2}, {2}}, {{-3}, {1}},
        {{-4}, list.pack(nil)},
        {{1, 2}, {1, 2}}, {{2, 2}, {2, 3}}, {{3, 2}, list.pack(3, nil)},
        {{0, 5}, list.pack(nil, 1, 2, 3, nil)},
    } do
        local args, want = table.unpack(test)
        t:expect(t:mexpr(a):get(table.unpack(args))):eq(want)
    end
end

function test_set(t)
    local zero = array('j', 0)
    t:expect(t:expr(zero):set(1)):eq(array('j', 0))
    t:expect(t:expr(zero):set(2)):raises("out of bounds")
    for _, test in ipairs{
        {{0}, nil},
        {{1}, {1, 2, 3}},
        {{1, 5}, {5, 2, 3}},
        {{2, 5}, {1, 5, 3}},
        {{3, 5}, {1, 2, 5}},
        {{4, 5}, {1, 2, 3}},
        {{5, 5}, nil},
        {{-1, 5}, {1, 2, 5}},
        {{-2, 5}, {1, 5, 3}},
        {{-3, 5}, {5, 2, 3}},
        {{-4, 5}, nil},
        {{2, 6, 7, 8}, {1, 6, 7}},
        {{-1, 6, 7}, {1, 2, 6}},
        {{-1, 6, 7, 8}, nil},
    } do
        local args, want = table.unpack(test)
        local a = array('j', 3, 4):set(1, 1, 2, 3, 4)
        local exp = t:expect(t:expr(a):set(table.unpack(args)))
        if want then exp:eq(array('j', #want):set(1, table.unpack(want)))
        else exp:raises("out of bounds") end
    end
end

function test_append(t)
    for _, test in ipairs{
        {array('j', 0), {}, {}},
        {array('j', 0), {1}, nil},
        {array('j', 0, 4), {1, 2, 3}, {1, 2, 3}},
        {array('j', 2, 4):set(1, 1, 2), {3}, {1, 2, 3}},
        {array('j', 2, 4):set(1, 1, 2), {3, 4}, {1, 2, 3, 4}},
        {array('j', 2, 4):set(1, 1, 2), {3, 4}, {1, 2, 3, 4}},
        {array('j', 2), {1}, nil},
    } do
        local a, args, want = table.unpack(test)
        local exp = t:expect(t:expr(a):append(table.unpack(args)))
        if want then exp:eq(array('j', #want):set(1, table.unpack(want)))
        else exp:raises("out of capacity") end
    end
end

function test_fill(t)
    for _, test in ipairs{
        {array('j', 0), {0}, {}},
        {array('j', 0), {0, 1}, {}},
        {array('j', 0), {0, 1, 0}, {}},
        {array('j', 0, 3):set(1, 1, 2, 3), {0}, {0, 0, 0}},
        {array('j', 0, 3):set(1, 1, 2, 3), {7, 2}, {1, 7, 7}},
        {array('j', 0, 3):set(1, 1, 2, 3), {7, 2, 1}, {1, 7, 3}},
        {array('j', 0, 3):set(1, 1, 2, 3), {7, 4}, {1, 2, 3}},
        {array('j', 0, 3):set(1, 1, 2, 3), {7, 4, 0}, {1, 2, 3}},
        {array('j', 0, 3):set(1, 1, 2, 3), {7, 4, -1}, {1, 2, 3}},
        {array('j', 0, 3):set(1, 1, 2, 3), {7, -1}, {1, 2, 7}},
        {array('j', 3), {7, 0}, nil},
        {array('j', 3), {7, 5}, nil},
        {array('j', 3), {7, 2, 3}, nil},
    } do
        local a, args, want = table.unpack(test)
        local exp = t:expect(t:expr(a):fill(table.unpack(args)))
        if want then
            a:len(a:cap())
            exp:eq(array('j', #want):set(1, table.unpack(want)))
        else exp:raises("out of bounds") end
    end
end
