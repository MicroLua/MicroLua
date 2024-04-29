-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local mem = require 'mlua.mem'
local string = require 'string'

function test_Buffer(t)
    local buf = mem.alloc(10)
    t:expect(t.expr(buf):addr()):neq(0)
    t:expect(#buf):label("#buf"):eq(10)
    t:expect(tostring(buf)):label("tostring(buf)")
        :matches('^mlua.mem.Buffer: 0?x?[0-9a-fA-F]+$')

    mem.fill(buf)
    t:expect(t.expr(mem).read(buf)):eq('\0\0\0\0\0\0\0\0\0\0')
    mem.fill(buf, ('_'):byte())
    t:expect(t.expr(mem).read(buf)):eq('__________')
    mem.fill(buf, ('z'):byte(), 7)
    t:expect(t.expr(mem).read(buf)):eq('_______zzz')
    mem.fill(buf, ('y'):byte(), 4, 2)
    t:expect(t.expr(mem).read(buf)):eq('____yy_zzz')
    mem.fill(buf, ('x'):byte(), nil, 3)
    t:expect(t.expr(mem).read(buf)):eq('xxx_yy_zzz')
    t:expect(t.expr(mem).fill(buf, 0, 11)):raises("out of bounds")
    t:expect(t.expr(mem).fill(buf, 0, 10, 1)):raises("out of bounds")

    mem.fill(buf, ('_'):byte())
    mem.write(buf, 'abc')
    t:expect(t.expr(mem).read(buf)):eq('abc_______')
    mem.write(buf, 'def', 3)
    t:expect(t.expr(mem).read(buf)):eq('abcdef____')
    t:expect(t.expr(mem).write(buf, 'x', 10)):raises("out of bounds")

    t:expect(t.expr(mem).read(buf, 3)):eq('def____')
    t:expect(t.expr(mem).read(buf, 3, 4)):eq('def_')
    t:expect(t.expr(mem).read(buf, 11)):raises("out of bounds")
    t:expect(t.expr(mem).read(buf, 10, 1)):raises("out of bounds")
end

function test_memory(t)
    local buf = mem.alloc(26)
    for _, test in ipairs{
        'abcdefghijklmnopqrstuvwxyz',
        '1234567890',
    } do
        mem.write(buf:addr(), test)
        t:expect(t.expr(mem).read(buf:addr(), #test)):eq(test)
    end
end
