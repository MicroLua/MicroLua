-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local math = require 'math'
local array = require 'mlua.array'
local int64 = require 'mlua.int64'
local list = require 'mlua.list'
local mem = require 'mlua.mem'
local repr = require 'mlua.repr'
local string = require 'string'
local table = require 'table'

function test_buffer(t)
    local a = array('j', 0)
    local ptr, len = a:__buffer()
    t:expect(t.expr(a):ptr()):eq(ptr)
    t:expect(#a):label("#a"):eq(len // ('j'):packsize())
end

function test_size(t)
    local _ = array  -- Capture the upvalue
    t:expect(t.expr.array('i0', 0)):raises("out of limits")
    t:expect(t.expr.array('i9', 0)):raises("out of limits")
    t:expect(t.expr.array('c0', 0)):raises("invalid value format")
    t:expect(t.expr.array('c999999999999999999999999', 0))
        :raises("invalid value format")
    t:expect(t.expr.array('a', 0)):raises("invalid value format")
    t:expect(t.expr.array('bb', 0)):raises("invalid value format")
    for _, typ in ipairs{
        'b', 'B', 'h', 'H', 'i', 'I', 'l', 'L', 'j', 'J', 'T',
        'i1', 'i2', 'i3', 'i4', 'i5', 'i6', 'i7', 'i8',
        'I1', 'I2', 'I3', 'I4', 'I5', 'I6', 'I7', 'I8',
        'f', 'd', 'n',
        'c1', 'c2', 'c4', 'c8', 'c16', 'c32', 'c64', 'c12345678',
    } do
        t:expect(t.expr.array(typ, 0):size()):eq(typ:packsize())
    end
end

local function clamp(typ, value)
    local size = typ:packsize()
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
    for _, typ in ipairs{
        'b', 'B', 'h', 'H', 'i', 'I', 'l', 'L', 'j', 'J', 'T',
        'i1', 'i2', 'i3', 'i4', 'I1', 'I2', 'I3', 'I4',
    } do
        t:context({type = typ})
        local a = array(typ, #values):set(1, table.unpack(values))
        local want = {}
        for i, v in ipairs(values) do want[i] = clamp(typ, v) end
        t:expect(t.mexpr(a):get(1, #a)):eq(want)
    end
end

local one64 = int64(1)

local function clamp64(typ, value)
    local size = typ:packsize()
    if typ < 'a' then return value & ((one64 << 8 * size) - 1) end
    local mask = (one64 << 8 * size) - 1
    value = value & mask
    if int64.ult(mask >> 1, value) then value = value - mask - 1 end
    return value
end

function test_int64(t)
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
    for _, typ in ipairs{'i5', 'i6', 'i7', 'i8', 'I5', 'I6', 'I7', 'I8'} do
        t:context({type = typ})
        local a = array(typ, #values):set(1, table.unpack(values))
        local want = {}
        for i, v in ipairs(values) do want[i] = clamp64(typ, v) end
        t:expect(t.mexpr(a):get(1, #a)):eq(want)
    end
end

local function pack_unpack(typ, value) return typ:unpack(typ:pack(value)) end

function test_float(t)
    local values = {
        0.0, 1.0, -1.0, 1.2e34, -1.2e34, 1.2e-34, -1.2e-34,
        math.huge, -math.huge,
    }
    for _, typ in ipairs{'f', 'd', 'n'} do
        t:context({type = typ})
        local a = array(typ, #values):set(1, table.unpack(values))
        local want = {}
        for i, v in ipairs(values) do want[i] = pack_unpack(typ, v) end
        t:expect(t.mexpr(a):get(1, #a)):eq(want)
    end
end

local function pad_string(typ, value)
    local size, len = tonumber(typ:sub(2), 10), #value
    if len > size then return value:sub(1, size) end
    return value .. ('\0'):rep(size - len)
end

function test_string(t)
    local values = {'', '123', 'abcde', 'the quick brown fox'}
    for _, typ in ipairs{'c1', 'c5', 'c11'} do
        t:context({type = typ})
        local a = array(typ, #values):set(1, table.unpack(values))
        local want = {}
        for i, v in ipairs(values) do want[i] = pad_string(typ, v) end
        t:expect(t.mexpr(a):get(1, #a)):eq(want)
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
        {array('n', 2):set(1, 21.0, -22.0),
         array('j', 2):set(1, 21, -22), true},
        {array('n', 2):set(1, 21.0, -22.0),
         array('j', 2):set(1, 21, 99), false},
        {array('n', 2):set(1, 21.0, -22.1),
         array('j', 2):set(1, 21, -22), false},
        {array('c1', 0), array('j', 0), true},
        {array('c1', 1):set(1, '1'), array('j', 1):set(1, 1), false},
    } do
        local a1, a2, want = table.unpack(test)
        t:expect(t.expr(_G).equal(a1, a2)):eq(want)
        t:expect(a1 == a2):label("%s == %s", a1, a2):eq(want)
    end
end

function test_buffer(t)
    for _, typ in ipairs{
        'b', 'B', 'h', 'H', 'i', 'I', 'j', 'J',
        'i1', 'i2', 'i3', 'i4', 'I1', 'I2', 'I3', 'I4',
        'f', 'd', 'n',
    } do
        t:context({type = typ})
        local a = array(typ, 4):set(1, 1, 2, 3, 4)
        local b = ''
        for _, v in ipairs(a) do b = b .. typ:pack(v) end
        t:expect(t.expr(mem).read(a)):eq(b)
        mem.write(a, typ:pack(67) .. typ:pack(89), 2 * typ:packsize())
        t:expect(a):eq(array(typ, 4):set(1, 1, 2, 67, 89))
    end
end

function test_repr(t)
    local _ = repr  -- Capture the upvalue
    for _, test in ipairs{
        {array('j', 0), '{}'},
        {array('j', 1):set(1, 1), '{1}'},
        {array('j', 3, 4):set(1, 1, -2, 3, 4), '{1, -2, 3}'},
        {array('n', 3, 4):set(1, 1.2, -3.4, 5.6, 7.8),
         ('{%s, %s, %s}'):format(1.2, -3.4, 5.6)},
        {array('c3', 3):set(1, 'abc', 'de', 'f'),
         '{"abc", "de\\x00", "f\\x00\\x00"}'},
    } do
        local value, want = table.unpack(test)
        t:expect(t.expr.repr(value)):eq(want)
    end
end

function test_index(t)
    local a = array('j', 3, 4):set(1, 1, 2, 3, 4)
    for _, test in ipairs{
        {0, nil}, {1, 1}, {2, 2}, {3, 3}, {4, nil},
        {-1, 3}, {-2, 2}, {-3, 1}, {-4, nil},
    } do
        local arg, want = table.unpack(test)
        t:expect(t.expr(a)[arg]):eq(want)
    end
end

function test_newindex(t)
    for _, test in ipairs{
        {0, 99, nil},
        {1, 99, {99, 2, 3, 4}},
        {2, 99, {1, 99, 3, 4}},
        {3, 99, {1, 2, 99, 4}},
        {4, 99, {1, 2, 3, 99}},
        {5, 99, nil},
        {-1, 99, {1, 2, 99, 4}},
        {-2, 99, {1, 99, 3, 4}},
        {-3, 99, {99, 2, 3, 4}},
        {-4, 99, nil},
    } do
        local a = array('j', 3, 4):set(1, 1, 2, 3, 4)
        local arg, value, want = table.unpack(test)
        local fn = function() a[arg] = value end
        if want then
            fn()
            a:len(a:cap())
            t:expect(a):label("a[%s] = %s", arg, value):op("=>")
                :eq(array('j', #want):set(1, table.unpack(want)))
        else
            t:expect(t.expr(fn)()):label("a[%s] = %s", arg, value)
                :raises("out of bounds")
        end
    end
end

function test_pairs(t)
    for _, fname in ipairs{'pairs', 'ipairs'} do
        local fn = _G[fname]
        for _, test in ipairs{
            {array('j', 0, 3), {}, {}},
            {array('j', 3, 4):set(1, 4, 2, 8, 5), {1, 2, 3}, {4, 2, 8}},
        } do
            local a, want_is, want_vs = table.unpack(test)
            local is, vs = list(), list()
            for i, v in fn(a) do
                is:append(i)
                vs:append(v)
            end
            t:expect(is):label("%s(%s) indexes", fname, a):eq(want_is)
            t:expect(vs):label("%s(%s) values", fname, a):eq(want_vs)
        end
    end
end

function test_get(t)
    local a = array('j', 2, 4):set(1, 1, 2, 3, 4)
    t:expect(t.expr(a):len(0)):eq(2)
    t:expect(t.expr(a):len(3)):eq(0)
    t:expect(t.expr(a):len(-1)):raises("invalid length")
    t:expect(t.expr(a):len(5)):raises("invalid length")
    t:expect(t.expr(a):len()):eq(3)
    t:expect(#a):label("#a"):eq(3)
    t:expect(t.expr(a):cap()):eq(4)
    for _, test in ipairs{
        {{0}, list.pack(nil)}, {{1}, {1}}, {{2}, {2}}, {{3}, {3}},
        {{4}, list.pack(nil)}, {{-1}, {3}}, {{-2}, {2}}, {{-3}, {1}},
        {{-4}, list.pack(nil)},
        {{1, 2}, {1, 2}}, {{2, 2}, {2, 3}}, {{3, 2}, list.pack(3, nil)},
        {{0, 5}, list.pack(nil, 1, 2, 3, nil)},
    } do
        local args, want = table.unpack(test)
        t:expect(t.mexpr(a):get(table.unpack(args))):eq(want)
    end
end

function test_set(t)
    local zero = array('j', 0)
    t:expect(t.expr(zero):set(1)):eq(array('j', 0))
    t:expect(t.expr(zero):set(2)):raises("out of bounds")
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
        local exp = t:expect(t.expr(a):set(table.unpack(args)))
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
        local exp = t:expect(t.expr(a):append(table.unpack(args)))
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
        local exp = t:expect(t.expr(a):fill(table.unpack(args)))
        if want then
            a:len(a:cap())
            exp:eq(array('j', #want):set(1, table.unpack(want)))
        else exp:raises("out of bounds") end
    end
end
