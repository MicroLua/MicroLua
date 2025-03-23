-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local io = require 'mlua.io'
local oo = require 'mlua.oo'
local table = require 'table'

function test_read(t)
    Reader = oo.class('Reader')
    function Reader:read(arg) return tostring(arg) end
    t:patch(_G, 'stdin', Reader())

    t:expect(t.expr(io).read(1234)):eq('1234')
end

function test_write(t)
    local r = t:patch(_G, 'stdout', io.Recorder())

    io.write('12', '|34')
    io.printf('|%s|%d', '56', 78)
    io.fprintf(r, '|%s', 90)
    t:expect(t.expr.tostring(r)):eq('12|34|56|78|90')
end

function test_Recorder(t)
    local r = io.Recorder()
    t:expect(t.expr(r):is_empty()):eq(true)
    r:write()
    r:write('', '', '')
    t:expect(t.expr(r):is_empty()):eq(true)
    local got = tostring(r)
    t:expect(t.expr.tostring(r)):eq('')

    r:write('foo', 'bar', '', 'baz')
    r:write('quux')
    t:expect(t.expr(r):is_empty()):eq(false)
    t:expect(t.expr.tostring(r)):eq('foobarbazquux')

    local b2 = io.Recorder()
    r:replay(b2)
    t:expect(t.expr.tostring(b2)):eq(tostring(r))
end

function test_Indenter(t)
    for _, test in ipairs{
        {'--', {}, ''},
        {'--', {{}}, ''},
        {'', {{'a\nb\nc\n'}, {'d\ne\nf', '\ng\nh'}}, 'a\nb\nc\nd\ne\nf\ng\nh'},
        {'--', {{'\na\n\nb\nc\n'}}, '\n--a\n\n--b\n--c\n'},
        {'--', {{'a\nb', 'c\n', 'd\n'}}, '--a\n--bc\n--d\n'},
        {'--', {{'a\nb'}, {'c\n'}, {'d\n'}}, '--a\n--bc\n--d\n'},
        {'--', {{'a\n', '\nb\n'}}, '--a\n\n--b\n'},
    } do
        local indent, writes, want = table.unpack(test)
        t:context({indent = indent, writes = writes})
        local r = io.Recorder()
        local ind = io.Indenter(r, indent)
        for _, args in ipairs(writes) do ind:write(table.unpack(args)) end
        t:expect(tostring(r)):label("result"):eq(want)
    end
end
