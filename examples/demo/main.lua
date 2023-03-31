_ENV = require "module"(...)

local math = require "math"
local string = require "string"

function main()
    print("In main()")
    print(string.format("maxinteger: %d", math.maxinteger))
    print(string.format("huge: %f", math.huge))

    -- local i = 0
    -- while true do
    --   print(string.format('Hello, MicroLua! (%d)', i))
    --   i = i + 1
    -- end
end
