-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local mem = require 'mlua.mem'
local string = require 'string'

function test_Buffer(t)
    local buf = mem.alloc(10)
    t:expect(buf:addr() ~= 0, "buffer address is 0")
    t:expect(#buf):label("#buf"):eq(10)
    t:expect(tostring(buf)):label("tostring(buf)")
        :eq(('mlua.mem.Buffer: %08X'):format(buf:addr()))

    buf:fill()
    t:expect(t:expr(buf):read()):eq('\0\0\0\0\0\0\0\0\0\0')
    buf:fill(('_'):byte())
    t:expect(t:expr(buf):read()):eq('__________')
    buf:fill(('z'):byte(), 7)
    t:expect(t:expr(buf):read()):eq('_______zzz')
    buf:fill(('y'):byte(), 4, 2)
    t:expect(t:expr(buf):read()):eq('____yy_zzz')
    buf:fill(('x'):byte(), nil, 3)
    t:expect(t:expr(buf):read()):eq('xxx_yy_zzz')
    t:expect(t:expr(buf):fill(0, 11)):raises("out of bounds")
    t:expect(t:expr(buf):fill(0, 10, 1)):raises("out of bounds")

    buf:fill(('_'):byte())
    buf:write('abc')
    t:expect(t:expr(buf):read()):eq('abc_______')
    buf:write('def', 3)
    t:expect(t:expr(buf):read()):eq('abcdef____')
    t:expect(t:expr(buf):write('x', 10)):raises("out of bounds")

    t:expect(t:expr(buf):read(3)):eq('def____')
    t:expect(t:expr(buf):read(3, 4)):eq('def_')
    t:expect(t:expr(buf):read(11)):raises("out of bounds")
    t:expect(t:expr(buf):read(10, 1)):raises("out of bounds")
end

function test_read_write(t)
    local buf = mem.alloc(26)
    for _, test in ipairs{
        'abcdefghijklmnopqrstuvwxyz',
        '1234567890',
    } do
        mem.write(buf:addr(), test)
        t:expect(t:expr(mem).read(buf:addr(), #test)):eq(test)
    end
end
