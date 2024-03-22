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
    local b = io.Recorder()
    t:patch(_G, 'stdout', b)

    io.write('12', '|34')
    io.printf('|%s|%d', '56', 78)
    io.fprintf(b, '|%s', 90)
    t:expect(t.expr.tostring(b)):eq('12|34|56|78|90')
end

function test_Recorder(t)
    local r = io.Recorder()
    t:expect(r:is_empty(), "New Recorder doesn't report empty")
    r:write()
    r:write('', '', '')
    t:expect(r:is_empty(), "Recorder with empty writes doesn't report empty")
    local got = tostring(r)
    t:expect(got == '', "Empty Recorder returns non-empty value: %q", got)

    r:write('foo', 'bar', '', 'baz')
    r:write('quux')
    t:expect(not r:is_empty(), "Non-empty Recorder reports empty")
    t:expect(t.expr.tostring(r)):eq('foobarbazquux')

    local b2 = io.Recorder()
    r:replay(b2)
    t:expect(t.expr.tostring(b2)):eq(tostring(r))
end
