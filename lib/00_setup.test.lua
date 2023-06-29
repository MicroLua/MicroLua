_ENV = mlua.Module(...)

local stdlib = require 'pico.stdlib'

function test_setup(t)
    stdlib.set_sys_clock_khz(250000, true)
    stdlib.setup_default_uart()
end
