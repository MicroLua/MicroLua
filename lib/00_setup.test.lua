_ENV = mlua.Module(...)

local uart = require 'hardware.uart'
local stdlib = require 'pico.stdlib'

function set_up(t)
    -- Set a faster clock speed.
    if uart.default then uart.default:tx_wait_blocking() end
    stdlib.set_sys_clock_khz(250000, true)
    stdlib.setup_default_uart()
end
