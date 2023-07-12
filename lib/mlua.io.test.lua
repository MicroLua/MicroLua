_ENV = mlua.Module(...)

local io = require 'mlua.io'
local oo = require 'mlua.oo'

function test_read(t)
    Reader = oo.class('Reader')
    function Reader:read(arg) return tostring(arg) end
    t:patch(_G, 'stdin', Reader())

    local got = io.read(1234)
    t:expect(got == '1234', "Unexpected input: got %q, want \"1234\"", got)
end

function test_write(t)
    local b = io.Buffer()
    t:patch(_G, 'stdout', b)

    io.write('12', '|34')
    io.printf('|%s|%d', '56', 78)
    io.fprintf(b, '|%s', 90)
    local got, want = tostring(b), '12|34|56|78|90'
    t:expect(got == want, "Unexpected output: got %q, want %q", got, want)
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
    local got, want = tostring(b), 'foobarbazquux'
    t:expect(got == want, "Unexpected buffer value: got %q, want %q", got, want)

    local b2 = io.Buffer()
    b:replay(b2)
    local got, want = tostring(b2), tostring(b)
    t:expect(got == want, "Unexpected replay: got %q, want %q", got, want)
end
