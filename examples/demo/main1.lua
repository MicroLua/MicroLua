_ENV = mlua.Module(...)

local timer = require 'hardware.timer'
local io = require 'mlua.io'
local thread = require 'mlua.thread'

function main()
    local start = thread.now()
    io.printf("Core 1 start time: %s\n", start)
    timer.busy_wait_ms(500)
    while true do
        local now = thread.now()
        io.printf("C1: main loop at %s\n", now)
        timer.busy_wait_ms(1000)
    end
end
