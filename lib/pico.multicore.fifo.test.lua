_ENV = mlua.Module(...)

local thread = require 'mlua.thread'
local multicore = require 'pico.multicore'
local fifo = require 'pico.multicore.fifo'

local module_name = ...

local function watch_shutdown()
    thread.start(function()
        multicore.wait_shutdown()
        thread.shutdown()
    end)
end

function test_status(t)
    fifo.enable_irq()
    t:cleanup(function() fifo.enable_irq(false) end)

    -- Initial state
    fifo.drain()
    fifo.clear_irq()
    t:expect(t:expr(fifo).rvalid()):eq(false)
    t:expect(t:expr(fifo).wready()):eq(true)
    t:expect(t:expr(fifo).get_status()):eq(fifo.RDY)

    -- Read status
    multicore.launch_core1(module_name, 'core1_push_one')
    t:cleanup(multicore.reset_core1)
    while not fifo.rvalid() do thread.yield() end
    t:expect(t:expr(fifo).get_status()):eq(fifo.RDY | fifo.VLD)
    t:expect(t:expr(fifo).pop_timeout_us(100)):eq(1)
    t:expect(t:expr(fifo).rvalid()):eq(false)
    t:expect(t:expr(fifo).get_status()):eq(fifo.RDY)
    t:expect(t:expr(fifo).pop_timeout_us(100)):eq(nil)

    -- Write status
    while fifo.wready() do
        t:expect(t:expr(fifo).push_timeout_us(0, 100)):eq(true)
    end
    t:expect(t:expr(fifo).push_timeout_us(0, 100)):eq(false)
    t:expect(t:expr(fifo).get_status()):eq(0)
end

function core1_push_one()
    watch_shutdown()
    fifo.enable_irq()
    local done<close> = function() fifo.enable_irq(false) end
    fifo.push_blocking(1)
end

function test_status_noyield(t)
    local save = yield_enabled(false)
    t:cleanup(function() yield_enabled(save) end)
    test_status(t)
end

function test_push_pop(t)
    multicore.launch_core1(module_name, 'core1_echo')
    t:cleanup(multicore.reset_core1)

    fifo.enable_irq()
    t:cleanup(function() fifo.enable_irq(false) end)
    for i = 1, 10 do
        fifo.push_blocking(i)
        t:expect(t:expr(fifo).pop_blocking()):eq(i)
    end
end

function core1_echo()
    watch_shutdown()
    fifo.enable_irq()
    local done<close> = function() fifo.enable_irq(false) end
    while true do
        fifo.push_blocking(fifo.pop_blocking())
    end
end

function test_push_pop_noyield(t)
    local save = yield_enabled(false)
    t:cleanup(function() yield_enabled(save) end)
    test_push_pop(t)
end
