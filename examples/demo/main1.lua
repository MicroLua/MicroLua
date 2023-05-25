_ENV = require 'mlua.module'(...)

local eio = require 'mlua.eio'
local time = require 'pico.time'

function main()
    local start = time.get_absolute_time()
    eio.printf("Core 1 start time: %s\n", start)
    time.sleep_ms(500)
    while true do
        local now = time.get_absolute_time()
        eio.printf("C1: main loop at %s\n", now)
        time.sleep_ms(1000)
    end
end
