_ENV = mlua.Module(...)

local debug = require 'debug'
local io = require 'mlua.io'
local thread = require 'mlua.thread'
local multicore = require 'pico.multicore'
local string = require 'string'

local module_name = ...

local function watch_shutdown()
    thread.start(function()
        multicore.wait_shutdown()
        thread.shutdown()
    end)
end

function test_core1(t)
    for i = 1, 3 do
        multicore.launch_core1(module_name, 'core1_wait_shutdown')
        multicore.reset_core1()
    end
end

function core1_wait_shutdown()
    watch_shutdown()
end

function test_fifo_status(t)
    multicore.fifo_enable_irq()
    t:cleanup(function() multicore.fifo_enable_irq(false) end)

    -- Initial state
    multicore.fifo_drain()
    multicore.fifo_clear_irq()
    t:expect(t:expr(multicore).fifo_rvalid()):eq(false)
    t:expect(t:expr(multicore).fifo_wready()):eq(true)
    t:expect(t:expr(multicore).fifo_get_status()):eq(multicore.FIFO_RDY)

    -- Read status
    multicore.launch_core1(module_name, 'core1_push_one')
    t:cleanup(multicore.reset_core1)
    while not multicore.fifo_rvalid() do thread.yield() end
    t:expect(t:expr(multicore).fifo_get_status())
        :eq(multicore.FIFO_RDY | multicore.FIFO_VLD)
    t:expect(t:expr(multicore).fifo_pop_timeout_us(100)):eq(1)
    t:expect(t:expr(multicore).fifo_rvalid()):eq(false)
    t:expect(t:expr(multicore).fifo_get_status()):eq(multicore.FIFO_RDY)
    t:expect(t:expr(multicore).fifo_pop_timeout_us(100)):eq(nil)

    -- Write status
    while multicore.fifo_wready() do
        t:expect(t:expr(multicore).fifo_push_timeout_us(0, 100)):eq(true)
    end
    t:expect(t:expr(multicore).fifo_push_timeout_us(0, 100)):eq(false)
    t:expect(t:expr(multicore).fifo_get_status()):eq(0)
end

function core1_push_one()
    watch_shutdown()
    multicore.fifo_enable_irq()
    local done<close> = function() multicore.fifo_enable_irq(false) end
    multicore.fifo_push_blocking(1)
end

function test_fifo_status_noyield(t)
    local save = yield_enabled(false)
    t:cleanup(function() yield_enabled(save) end)
    test_fifo_status(t)
end

function test_fifo_push_pop(t)
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

function test_fifo_push_pop_noyield(t)
    local save = yield_enabled(false)
    t:cleanup(function() yield_enabled(save) end)
    test_fifo_push_pop(t)
end
