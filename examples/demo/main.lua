_ENV = require "module"(...)

local math = require "math"
local string = require "string"

function main()
    print("In main()")
    print(string.format("max integer: %d", math.maxinteger))
    print(string.format("max float: %g", 3.402823466e38))
    print(string.format("max double: %g", 1.7976931348623158e308))

    -- local i = 0
    -- while true do
    --   print(string.format('Hello, MicroLua! (%d)', i))
    --   i = i + 1
    -- end
end
