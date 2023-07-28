_ENV = mlua.Module(...)

local debug = require 'debug'
local io = require 'mlua.io'
local thread = require 'mlua.thread'
local multicore = require 'pico.multicore'
local string = require 'string'

local module_name = ...
local core1 = {}

local function watch_shutdown()
    thread.start(function()
        multicore.wait_shutdown()
        thread.shutdown()
    end)
end

function test_core1(t)
    multicore.launch_core1(module_name, 'core1_wait_shutdown')
    multicore.reset_core1()
end

function core1_wait_shutdown()
    watch_shutdown()
end

function test_fifo(t)
    -- TODO: Test variants with timeouts
    -- TODO: Test other functions
    multicore.launch_core1(module_name, 'core1_fifo_echo')
    t:cleanup(multicore.reset_core1)

    multicore.fifo_enable_irq()
    t:cleanup(function() multicore.fifo_enable_irq(false) end)
    for i = 1, 10 do
        multicore.fifo_push_blocking(i)
        t:expect(t:expr(multicore).fifo_pop_blocking()):eq(i)
    end
end

function core1_fifo_echo()
    watch_shutdown()
    multicore.fifo_enable_irq()
    local done<close> = function() multicore.fifo_enable_irq(false) end
    while true do
        multicore.fifo_push_blocking(multicore.fifo_pop_blocking())
    end
end
