_ENV = mlua.Module(...)

local thread = require 'mlua.thread'
local multicore = require 'pico.multicore'

local module_name = ...

function test_core1(t)
    for i = 1, 3 do
        multicore.launch_core1(module_name, 'core1_wait_shutdown')
        multicore.reset_core1()
    end
end

function core1_wait_shutdown()
    multicore.wait_shutdown()
    thread.shutdown()
end
