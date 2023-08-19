_ENV = mlua.Module(...)

local io = require 'mlua.io'
local stdio = require 'mlua.stdio'

function test_print(t)
    local b = io.Buffer()
    t:patch(_G, 'stdout', b)

    local v = setmetatable({}, {__tostring = function() return '(v)' end})
    print(1, 2.3, "4-5", v)
    print(6, 7, 8)
    t:expect(tostring(b)):label('output')
        :eq("1\t" .. tostring(2.3) .."\t4-5\t(v)\n6\t7\t8\n")
end
