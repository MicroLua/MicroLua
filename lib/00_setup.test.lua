_ENV = mlua.Module(...)

local rtc = require 'hardware.rtc'
local uart = require 'hardware.uart'
local stdlib = require 'pico.stdlib'

function set_up(t)
    -- Start and set the RTC.
    rtc.init()
    t:expect(rtc.set_datetime({year = 2020, month = 1, day = 2, dotw = 4,
                               hour = 3, min = 4, sec = 0}),
             "failed to set RTC time")

    -- Set a faster clock speed.
    if uart.default then uart.default:tx_wait_blocking() end
    stdlib.set_sys_clock_khz(250000, true)
    stdlib.setup_default_uart()
end
