-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local thread = require 'mlua.thread'
local multicore = require 'pico.multicore'
local fifo = require 'pico.multicore.fifo'

local module_name = ...

function set_up(t)
    multicore.reset_core1()
    fifo.drain()
end

function test_status_BNB(t)
    if not thread.blocking() then
        fifo.enable_irq()
        t:cleanup(function() fifo.enable_irq(false) end)
    end

    -- Initial state
    fifo.drain()
    fifo.clear_irq()
    t:expect(t.expr(fifo).rvalid()):eq(false)
    t:expect(t.expr(fifo).wready()):eq(true)
    t:expect(t.expr(fifo).get_status()):eq(fifo.RDY)

    -- Read status
    multicore.launch_core1(module_name, 'core1_push_one')
    t:cleanup(multicore.reset_core1)
    while not fifo.rvalid() do thread.yield() end
    t:expect(t.expr(fifo).get_status()):eq(fifo.RDY | fifo.VLD)
    t:expect(t.expr(fifo).pop_timeout_us(100)):eq(1)
    t:expect(t.expr(fifo).rvalid()):eq(false)
    t:expect(t.expr(fifo).get_status()):eq(fifo.RDY)
    t:expect(t.expr(fifo).pop_timeout_us(100)):eq(nil)

    -- Write status
    while fifo.wready() do
        t:expect(t.expr(fifo).push_timeout_us(0, 100)):eq(true)
    end
    t:expect(t.expr(fifo).push_timeout_us(0, 100)):eq(false)
    t:expect(t.expr(fifo).get_status()):eq(0)
end

function core1_push_one()
    multicore.set_shutdown_handler()
    fifo.enable_irq()
    local done<close> = function() fifo.enable_irq(false) end
    fifo.push_blocking(1)
end

function test_push_pop_BNB(t)
    multicore.launch_core1(module_name, 'core1_echo')
    t:cleanup(multicore.reset_core1)
    if not thread.blocking() then
        fifo.enable_irq()
        t:cleanup(function() fifo.enable_irq(false) end)
    end
    for i = 1, 10 do
        fifo.push_blocking(i)
        t:expect(t.expr(fifo).pop_blocking()):eq(i)
    end
end

function core1_echo()
    multicore.set_shutdown_handler()
    fifo.enable_irq()
    local done<close> = function() fifo.enable_irq(false) end
    while true do
        fifo.push_blocking(fifo.pop_blocking())
    end
end
