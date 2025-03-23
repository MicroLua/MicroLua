-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local repr = require 'mlua.repr'
local table = require 'table'

function test_repr(t)
    local repr = repr
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
        {{1, nil, 3, n = 3}, '{1, nil, 3, n = 3}'},
        {{n = 3}, '{nil, nil, nil, n = 3}'},
        {{1, 2, n = -1}, '{[1] = 1, [2] = 2, n = -1}'},  -- Negative len
        {{[4] = 4, n = 3}, '{[4] = 4, n = 3}'},  -- Out-of-range key
        {{1, 2, n = 11}, '{[1] = 1, [2] = 2, n = 11}'},  -- Too many nils
        {{a = 1, b = 2, [3] = 4}, '{[3] = 4, a = 1, b = 2}'},
        {{a = {1, 2}, b = {c = 3, d = 4}}, '{a = {1, 2}, b = {c = 3, d = 4}}'},
        {rec, '{a = {b = 2, c = ..., d = 4}, e = ...}'},
    } do
        local v, want = table.unpack(test, 1, 2)
        t:expect(t.expr.repr(v)):eq(want)
    end
end
