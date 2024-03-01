-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local list = require 'mlua.list'
local util = require 'mlua.util'
local table = require 'table'

function test_repr(t)
    local rec = {a = {b = 2, d = 4}}
    rec.a.c = rec
    rec.e = rec
    for _, test in ipairs{
        {nil, tostring(nil)},
        {false, tostring(false)},
        {true, tostring(true)},
        {123, tostring(123)},
        {4.5, tostring(4.5)},
        {'abc', '"abc"'},
        {'\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f'
         .. '\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f'
         .. '\x7f"\\',
         '"\\x00\\x01\\x02\\x03\\x04\\x05\\x06\\a\\b\\t\\n\\v\\f\\r\\x0e\\x0f'
         .. '\\x10\\x11\\x12\\x13\\x14\\x15\\x16\\x17\\x18\\x19\\x1a\\x1b\\x1c'
         .. '\\x1d\\x1e\\x1f\\x7f\\"\\\\"'},
        {{}, '{}'},
        {{1, 2}, '{1, 2}'},
        {{[0] = 3, 1, nil, 3}, '{[0] = 3, 1, nil, 3}'},
        {{[0] = 3}, '{[0] = 3, nil, nil, nil}'},
        {{[0] = -1, 1, 2}, '{[0] = -1, [1] = 1, [2] = 2}'},  -- Negative len
        {{[0] = 3, [4] = 4}, '{[0] = 3, [4] = 4}'},  -- Out-of-range key
        {{[0] = 11, 1, 2}, '{[0] = 11, [1] = 1, [2] = 2}'},  -- Too many nils
        {{a = 1, b = 2, [3] = 4}, '{[3] = 4, a = 1, b = 2}'},
        {{a = {1, 2}, b = {c = 3, d = 4}}, '{a = {1, 2}, b = {c = 3, d = 4}}'},
        {rec, '{a = {b = 2, c = ..., d = 4}, e = ...}'},
    } do
        local v, want = table.unpack(test)
        t:expect(t:expr(util).repr(v)):eq(want)
    end
end

function test_keys(t)
    for _, test in ipairs{
        {{}, nil, {[0] = 0}},
        {{a = 1, b = 2, c = 3}, nil, {[0] = 3, 'a', 'b', 'c'}},
        {{a = 4, b = 5, c = 6}, function(k, v) return k ~= 'b' and v ~= 4 end,
         {[0] = 1, 'c'}},
    } do
        local tab, filter, want = table.unpack(test)
        t:expect(t:expr(util).keys(tab, filter):sort()):eq(want, util.table_eq)
    end
end

function test_values(t)
    for _, test in ipairs{
        {{}, nil, {[0] = 0}},
        {{a = 1, b = 2, c = 3}, nil, {[0] = 3, 1, 2, 3}},
        {{a = 4, b = 5, c = 6}, function(k, v) return k ~= 'b' and v ~= 4 end,
         {[0] = 1, 6}},
    } do
        local tab, filter, want = table.unpack(test)
        t:expect(t:expr(util).values(tab, filter):sort())
            :eq(want, util.table_eq)
    end
end

function test_table_eq(t)
    for _, test in ipairs{
        {nil, nil, true},
        {{}, nil, false},
        {{}, {}, true},
        {{}, {1, 2}, false},
        {{1, 2}, {1, 3}, false},
        {{1, 2}, {1, 2}, true},
        {{a = 1, b = 2}, {a = 1, b = 2}, true},
        {{a = 1, b = 2}, {a = 1, c = 2}, false},
        {{a = 1, b = 2}, {a = 1, b = 3}, false},
    } do
        local a, b, want = table.unpack(test)
        t:expect(t:expr(util).table_eq(a, b)):eq(want)
        t:expect(t:expr(util).table_eq(b, a)):eq(want)
    end
end

function test_table_copy(t)
    for _, test in ipairs{
        {nil},
        {{}},
        {{1, 2, a = 3, b = 4}},
        {setmetatable({1, 2, a = 3, b = 4}, {5, 6})},
    } do
        local tab = table.unpack(test)
        t:expect(t:expr(util).table_copy(tab)):eq(tab, util.table_eq)
            :apply(getmetatable):op("metatable is"):eq(getmetatable(tab))
    end
end

function test_table_comp(t)
    local a, b = {10, 20, 30}, {30, 20, 10}
    for _, test in ipairs{
        {{1}, true}, {{2}, false}, {{3}, false},
        {{1, 2, 3}, true}, {{2, 2, 2}, false}, {{3, 2, 1}, false},
    } do
        local keys, want = table.unpack(test)
        t:expect(t:expr(util).table_comp(keys)(a, b))
    end
end
