-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local mem = require 'mlua.mem'
local string = require 'string'
local table = require 'table'

function test_misc(t)
    local buf = mem.alloc(10)
    t:expect(t.expr(buf):addr()):neq(0)
    t:expect(#buf):label("#buf"):eq(10)
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
        {{11}, nil},
        {{nil, 5}, 'abcde'},
        {{3, 5}, 'defgh'},
        {{7, 0}, ''},
        {{7, 3}, 'hij'},
        {{7, 4}, nil},
        {{10, 0}, ''},
        {{11, 0}, nil},
    } do
        local args, want = table.unpack(test)
        local exp = t:expect(t.expr(mem).read(buf, table.unpack(args)))
        if want then
            exp:eq(want)
            local off = args[1] or 0
            local len = args[2] or #buf - off
            t:expect(t.expr(mem).read(buf:addr() + off, len)):eq(want)
        else exp:raises("out of bounds") end
    end
end

function test_write(t)
    for _, test in ipairs{
        {{''}, '__________'},
        {{'abc'}, 'abc_______'},
        {{'abcdefghijk'}, nil},
        {{'abcde', 3}, '___abcde__'},
        {{'ab', 8}, '________ab'},
        {{'abc', 8}, nil},
        {{'', 10}, '__________'},
        {{'', 11}, nil},
    } do
        local args, want = table.unpack(test)
        local buf = mem.alloc(10)
        mem.write(buf, '__________')
        local exp = t:expect(t.expr(mem).write(buf, table.unpack(args)))
        if want then
            exp:eq(nil)
            t:expect(t.expr(mem).read(buf)):eq(want)
            mem.write(buf, '__________')
            local off = args[2] or 0
            mem.write(buf:addr() + off, args[1])
            t:expect(t.expr(mem).read(buf)):eq(want)
        else exp:raises("out of bounds") end
    end
end

function test_fill(t)
    for _, test in ipairs{
        {{}, '\0\0\0\0\0\0\0\0\0\0'},
        {{2}, '\2\2\2\2\2\2\2\2\2\2'},
        {{5, 3}, '___\5\5\5\5\5\5\5'},
        {{2, 10}, '__________'},
        {{2, 11}, nil},
        {{7, nil, 5}, '\7\7\7\7\7_____'},
        {{2, 3, 5}, '___\2\2\2\2\2__'},
        {{2, 7, 0}, '__________'},
        {{2, 7, 3}, '_______\2\2\2'},
        {{2, 7, 4}, nil},
        {{2, 10, 0}, '__________'},
        {{2, 11, 0}, nil},
    } do
        local args, want = table.unpack(test)
        local buf = mem.alloc(10)
        mem.write(buf, '__________')
        local exp = t:expect(t.expr(mem).fill(buf, table.unpack(args)))
        if want then
            exp:eq(nil)
            t:expect(t.expr(mem).read(buf)):eq(want)
            mem.write(buf, '__________')
            local off = args[2] or 0
            local len = args[3] or #buf - off
            mem.fill(buf:addr() + off, args[1], len)
            t:expect(t.expr(mem).read(buf)):eq(want)
        else exp:raises("out of bounds") end
    end
end

function test_get(t)
    local buf = mem.alloc(8)
    mem.write(buf, '\0\1\2\3\4\5\6\7')
    for _, test in ipairs{
        {{0}, {0}},
        {{3}, {3}},
        {{7}, {7}},
        {{8}, nil},
        {{0, 0}, {}},
        {{0, 3}, {0, 1, 2}},
        {{2, 4}, {2, 3, 4, 5}},
        {{5, 3}, {5, 6, 7}},
        {{5, 4}, nil},
        {{8, 0}, {}},
        {{9, 0}, nil},
    } do
        local args, want = table.unpack(test)
        local exp = t:expect(t.mexpr(mem).get(buf, table.unpack(args)))
        if want then
            exp:eq(want)
            local len = args[2] or 1
            t:expect(t.mexpr(mem).get(buf:addr() + args[1], len)):eq(want)
        else exp:raises("out of bounds") end
    end
end

function test_set(t)
    for _, test in ipairs{
        {{0}, '________'},
        {{0, 1, 2, 3}, '\1\2\3_____'},
        {{3, 4, 5, 6, 7}, '___\4\5\6\7_'},
        {{5, 2, 4, 6}, '_____\2\4\6'},
        {{5, 2, 4, 6, 8}, nil},
        {{8}, '________'},
        {{9}, nil},
    } do
        local args, want = table.unpack(test)
        local buf = mem.alloc(8)
        mem.write(buf, '________')
        local exp = t:expect(t.expr(mem).set(buf, table.unpack(args)))
        if want then
            exp:eq(nil)
            t:expect(t.expr(mem).read(buf)):eq(want)
            mem.write(buf, '________')
            mem.set(buf:addr() + args[1], table.unpack(args, 2))
            t:expect(t.expr(mem).read(buf)):eq(want)
        else exp:raises("out of bounds") end
    end
end
