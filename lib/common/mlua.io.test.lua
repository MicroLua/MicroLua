-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local io = require 'mlua.io'
local oo = require 'mlua.oo'

function test_read(t)
    Reader = oo.class('Reader')
    function Reader:read(arg) return tostring(arg) end
    t:patch(_G, 'stdin', Reader())

    t:expect(t:expr(io).read(1234)):eq('1234')
end

function test_write(t)
    local b = io.Buffer()
    t:patch(_G, 'stdout', b)

    io.write('12', '|34')
    io.printf('|%s|%d', '56', 78)
    io.fprintf(b, '|%s', 90)
    t:expect(t.expr.tostring(b)):eq('12|34|56|78|90')
end

function test_Buffer(t)
    local b = io.Buffer()
    t:expect(b:is_empty(), "New buffer doesn't report empty")
    b:write()
    b:write('', '', '')
    t:expect(b:is_empty(), "Buffer with empty writes doesn't report empty")
    local got = tostring(b)
    t:expect(got == '', "Empty buffer returns non-empty value: %q", got)

    b:write('foo', 'bar', '', 'baz')
    b:write('quux')
    t:expect(not b:is_empty(), "Non-empty buffer reports empty")
    t:expect(t.expr.tostring(b)):eq('foobarbazquux')

    local b2 = io.Buffer()
    b:replay(b2)
    t:expect(t.expr.tostring(b2)):eq(tostring(b))
end
