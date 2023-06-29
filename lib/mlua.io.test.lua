_ENV = mlua.Module(...)

local io = require 'mlua.io'

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
