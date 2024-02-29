-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local pio = require 'hardware.pio'
local addressmap = require 'hardware.regs.addressmap'
local list = require 'mlua.list'
local thread = require 'mlua.thread'
local time = require 'mlua.time'

-- TODO: Add a multicore test with Tx in one core and Rx in the other

function test_config(t)
    local cfg = pio.get_default_sm_config()
        :set_out_pins(1, 2)
        :set_set_pins(3, 1)
        :set_clkdiv_int_frac(250)
    -- TODO
end

function test_PIO_sm(t)
    local inst = pio[0]
    for i = 0, 3 do
        for j = 0, 3 do
            t:expect(inst:sm(i) == inst:sm(j))
                :label("inst:sm(%s) == inst:sm(%s)", i, j):eq(i == j)
        end
    end
end

function test_PIO_index_base(t)
    for i = 0, pio.NUM - 1 do
        local inst = pio[i]
        t:expect(t:expr(inst):get_index()):eq(i)
        t:expect(t:expr(inst):regs_base())
            :eq(addressmap[('PIO%s_BASE'):format(i)])
    end
end

local function setup(t, prog, irq_mask)
    -- Claim a state machine.
    local inst = pio[1]
    local sm = inst:sm(2)
    sm:claim();
    t:cleanup(function()
        sm:unclaim()
        t:expect(t:expr(sm):is_claimed()):eq(false)
    end)
    t:expect(t:expr(sm):index()):eq(2)
    t:expect(t:expr(sm):is_claimed()):eq(true)

    -- Enable IRQs if non-blocking behavior is enabled.
    if not thread.blocking() then
        local mask = (1 << (pio.pis_sm0_tx_fifo_not_full + sm:index()))
                     | (1 << (pio.pis_sm0_rx_fifo_not_empty + sm:index()))
                     | ((irq_mask or 0) << pio.pis_interrupt0)
        inst:enable_irq(mask)
        t:cleanup(function() inst:enable_irq(mask, false) end)
    end

    -- Load the program.
    t:assert(t:expr(inst):can_add_program(prog)):eq(true)
    local off = inst:add_program(prog)
    t:expect(off):label("offset"):eq(32 - #prog)
    t:cleanup(function() inst:remove_program(prog, off) end)

    local cfg = pio.get_default_sm_config()
        :set_wrap(prog.wrap_target + off, prog.wrap + off)
    return inst, sm, cfg, off
end

local pio_timer = {
    0xe040, 0xa04a, 0x80a0, 0xa027, 0x0044, 0xa0ca, 0x8020, 0x0082,
    labels = {start = 0}, wrap_target = 2, wrap = 7,
}

function test_put_BNB(t)
    local inst, sm, cfg, off = setup(t, pio_timer)
    cfg:set_clkdiv_int_frac(250)
    sm:init(pio_timer.labels.start + off, cfg)
    t:cleanup(function() sm:set_enabled(false) end)
    sm:set_enabled(true)

    local ticks = time.ticks
    for _, delay in ipairs{500, 1000, 2000} do
        sm:clear_fifos()
        local t1 = ticks()
        sm:put_blocking(delay)  -- Consumed immediately
        sm:put_blocking(1)      -- FIFO
        sm:put_blocking(1)      -- FIFO
        sm:put_blocking(1)      -- FIFO
        sm:put_blocking(1)      -- FIFO
        sm:put_blocking(1)      -- Blocks until the delay elapses
        local t2 = ticks()
        t:expect(t2 - t1):label("delay"):gte(delay):lt(delay + 200)
    end
end

function test_get_BNB(t)
    local inst, sm, cfg, off = setup(t, pio_timer)
    cfg:set_clkdiv_int_frac(250)
    sm:init(pio_timer.labels.start + off, cfg)
    t:cleanup(function() sm:set_enabled(false) end)
    sm:set_enabled(true)

    local ticks = time.ticks
    for i, delay in ipairs{500, 1000, 2000} do
        local t1 = ticks()
        sm:put_blocking(delay)
        local v = sm:get_blocking()
        local t2 = ticks()
        t:expect(v):label("get_blocking()"):eq(i - 1)
        t:expect(t2 - t1):label("delay"):gte(delay):lt(delay + 200)
    end
end

local pio_irqs = {
    0x80a0, 0xa027, 0xc020, 0xc021, 0xc022, 0xc023, 0x0042, 0x8020,
    labels = {start = 0}, wrap_target = 0, wrap = 7,
}

function test_irq_callback(t)
    local inst, sm, cfg, off = setup(t, pio_irqs,
                                     (1 << pio.NUM_STATE_MACHINES) - 1)
    sm:init(pio_irqs.labels.start + off, cfg)
    t:cleanup(function() sm:set_enabled(false) end)
    sm:set_enabled(true)

    local log = list()
    inst:set_irq_callback(function(pending)
        log:append(pending)
        t:expect(t:expr(inst):interrupt_get_mask()):eq(pending)
        for i = 0, pio.NUM_STATE_MACHINES - 1 do
            t:expect(t:expr(inst):interrupt_get(i))
                :eq((pending & (1 << i) ~= 0))
        end
        inst:interrupt_clear_mask(pending)
    end)
    t:cleanup(function() inst:set_irq_callback(nil) end)

    sm:put_blocking(2)
    sm:get_blocking()
    t:expect(log):label("log"):eq{1, 2, 4, 8, 1, 2, 4, 8, 1, 2, 4, 8}
end
