_ENV = require 'mlua.module'(...)

local timer = require 'hardware.timer'
local eio = require 'mlua.eio'
local thread = require 'mlua.thread'

function main()
    local start = thread.now()
    eio.printf("Core 1 start time: %s\n", start)
    timer.busy_wait_ms(500)
    while true do
        local now = thread.now()
        eio.printf("C1: main loop at %s\n", now)
        timer.busy_wait_ms(1000)
    end
end
