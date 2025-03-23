-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local list = require 'mlua.list'
local mem = require 'mlua.mem'
local string = require 'string'
local table = require 'table'

local raise = {}

function test_buffer(t)
    local buf = mem.alloc(10)
    local ptr, len = buf:__buffer()
    t:expect(t.expr(buf):ptr()):eq(ptr)
    t:expect(#buf):label("#buf"):eq(len)
    t:expect(t.expr(_G).tostring(buf))
        :matches('^mlua.mem.Buffer: 0?x?[0-9a-fA-F]+$')
end

function test_read(t)
    local buf = mem.alloc(10)
    mem.write(buf, 'abcdefghij')
    for _, test in ipairs{
        {{}, 'abcdefghij'},
        {{3}, 'defghij'},
        {{10}, ''},
        {{11}, raise},
        {{nil, 5, n = 2}, 'abcde'},
        {{3, 5}, 'defgh'},
        {{7, 0}, ''},
        {{7, 3}, 'hij'},
        {{7, 4}, raise},
        {{10, 0}, ''},
        {{11, 0}, raise},
    } do
        local args, want = table.unpack(test)
        local exp = t:expect(t.expr(mem).read(buf, list.unpack(args)))
        if want ~= raise then
            exp:eq(want)
            local off = args[1]
            local len = args[2] or #buf - (off or 0)
            t:expect(t.expr(mem).read(buf:ptr(), off, len)):eq(want)
        else exp:raises("out of bounds") end
    end
end

function test_read_cstr(t)
    local buf = mem.alloc(10)
    mem.write(buf, 'abc\0def\0gh')
    for _, test in ipairs{
        {{}, 'abc'},
        {{4}, 'def'},
        {{8}, 'gh'},
        {{10}, ''},
        {{11}, raise},
        {{nil, 2, n = 2}, 'ab'},
        {{4, 0}, ''},
        {{4, 2}, 'de'},
        {{8, 2}, 'gh'},
        {{8, 3}, raise},
        {{10, 0}, ''},
        {{11, 0}, raise},
    } do
        local args, want = table.unpack(test)
        local exp = t:expect(t.expr(mem).read_cstr(buf, list.unpack(args)))
        if want ~= raise then
            exp:eq(want)
            local off = args[1]
            local len = args[2] or #buf - (off or 0)
            t:expect(t.expr(mem).read_cstr(buf:ptr(), off, len)):eq(want)
        else exp:raises("out of bounds") end
    end
end

function test_write(t)
    local buf = mem.alloc(10)
    for _, test in ipairs{
        {{''}, '__________'},
        {{'abc'}, 'abc_______'},
        {{'abcdefghijk'}, raise},
        {{'abcde', 3}, '___abcde__'},
        {{'ab', 8}, '________ab'},
        {{'abc', 8}, raise},
        {{'', 10}, '__________'},
        {{'', 11}, raise},
    } do
        local args, want = table.unpack(test)
        mem.write(buf, '__________')
        local exp = t:expect(t.expr(mem).write(buf, table.unpack(args)))
        if want ~= raise then
            exp:eq(nil)
            t:expect(t.expr(mem).read(buf)):eq(want)
            mem.write(buf, '__________')
            mem.write(buf:ptr(), table.unpack(args))
            t:expect(t.expr(mem).read(buf)):eq(want)
        else exp:raises("out of bounds") end
    end
end

function _test_fill(t)
    local buf = mem.alloc(10)
    for _, test in ipairs{
        {{}, '\0\0\0\0\0\0\0\0\0\0'},
        {{2}, '\2\2\2\2\2\2\2\2\2\2'},
        {{5, 3}, '___\5\5\5\5\5\5\5'},
        {{2, 10}, '__________'},
        {{2, 11}, raise},
        {{7, nil, 5}, '\7\7\7\7\7_____'},
        {{2, 3, 5}, '___\2\2\2\2\2__'},
        {{2, 7, 0}, '__________'},
        {{2, 7, 3}, '_______\2\2\2'},
        {{2, 7, 4}, raise},
        {{2, 10, 0}, '__________'},
        {{2, 11, 0}, raise},
    } do
        local args, want = table.unpack(test)
        mem.write(buf, '__________')
        local exp = t:expect(t.expr(mem).fill(buf, table.unpack(args)))
        if want ~= raise then
            exp:eq(nil)
            t:expect(t.expr(mem).read(buf)):eq(want)
            mem.write(buf, '__________')
            mem.fill(buf:ptr(), table.unpack(args))
            t:expect(t.expr(mem).read(buf)):eq(want)
        else exp:raises("out of bounds") end
    end
end

function test_find(t)
    local buf = mem.alloc(10)
    mem.write(buf, 'abcdefabcd')
    for _, test in ipairs{
        {{'ab'}, 0},
        {{'bc'}, 1},
        {{'def'}, 3},
        {{'def', nil, 5, n = 3}, nil},
        {{'bc', 5}, 7},
        {{'bc', 5, 3}, nil},
        {{'xyz'}, nil},
        {{'def', 5}, nil},
        {{'cde', 8}, raise},
        {{''}, 0},
        {{'xyz', 7}, nil},
        {{'xyz', 8}, raise},
        {{'', 10}, 10},
        {{'', 11}, raise},
    } do
        local args, want = table.unpack(test, 1, 2)
        local exp = t:expect(t.expr(mem).find(buf, list.unpack(args)))
        if want ~= raise then
            exp:eq(want)
            local off = args[2]
            local len = args[3] or #buf - (off or 0)
            t:expect(t.expr(mem).find(buf:ptr(), args[1], off, len)):eq(want)
        else exp:raises("out of bounds") end
    end
end

function _test_get(t)
    local buf = mem.alloc(8)
    mem.write(buf, '\0\1\2\3\4\5\6\7')
    for _, test in ipairs{
        {{0}, {0}},
        {{3}, {3}},
        {{7}, {7}},
        {{8}, raise},
        {{0, 0}, {}},
        {{0, 3}, {0, 1, 2}},
        {{2, 4}, {2, 3, 4, 5}},
        {{5, 3}, {5, 6, 7}},
        {{5, 4}, raise},
        {{8, 0}, {}},
        {{9, 0}, raise},
    } do
        local args, want = table.unpack(test)
        local exp = t:expect(t.mexpr(mem).get(buf, table.unpack(args)))
        if want ~= raise then
            exp:eq(want)
            t:expect(t.mexpr(mem).get(buf:ptr(), table.unpack(args))):eq(want)
        else exp:raises("out of bounds") end
    end
end

function _test_set(t)
    local buf = mem.alloc(8)
    for _, test in ipairs{
        {{0}, '________'},
        {{0, 1, 2, 3}, '\1\2\3_____'},
        {{3, 4, 5, 6, 7}, '___\4\5\6\7_'},
        {{5, 2, 4, 6}, '_____\2\4\6'},
        {{5, 2, 4, 6, 8}, raise},
        {{8}, '________'},
        {{9}, raise},
    } do
        local args, want = table.unpack(test)
        mem.write(buf, '________')
        local exp = t:expect(t.expr(mem).set(buf, table.unpack(args)))
        if want ~= raise then
            exp:eq(nil)
            t:expect(t.expr(mem).read(buf)):eq(want)
            mem.write(buf, '________')
            mem.set(buf:ptr(), table.unpack(args))
            t:expect(t.expr(mem).read(buf)):eq(want)
        else exp:raises("out of bounds") end
    end
end
