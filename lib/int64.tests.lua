_ENV = require 'module'(...)

local int64 = require 'int64'

function test_unary_ops(t)
end

function test_binary_ops(t)
    t:expect(int64(1) + int64(2) == int64(3), "1 + 2 == 3")
end
