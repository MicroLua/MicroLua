-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local pio = require 'hardware.pio'
local addressmap = require 'hardware.regs.addressmap'
local list = require 'mlua.list'
local thread = require 'mlua.thread'
local time = require 'mlua.time'
local string = require 'string'

-- TODO: Add a multicore test with Tx in one core and Rx in the other

local function hex8(v) return ('0x%08x'):format(v) end

function test_config(t)
    local cfg = pio.get_default_sm_config()
        :set_out_pins(1, 2)
        :set_set_pins(3, 4)
        :set_in_pins(5)
        :set_sideset_pins(6)
        :set_sideset(1, true, true)
        :set_clkdiv_int_frac(123, 45)
        :set_wrap(7, 13)
        :set_jmp_pin(8)
        :set_in_shift(true, true, 9)
        :set_out_shift(true, true, 10)
        :set_fifo_join(pio.FIFO_JOIN_RX)
        :set_out_special(true, true, 11)
        :set_mov_status(pio.STATUS_RX_LESSTHAN, 5)
    t:expect(t.expr(cfg):clkdiv()):fmt(hex8):eq((123 << 16) | (45 << 8))
    t:expect(t.expr(cfg):execctrl()):fmt(hex8)
        :eq(0x60060010 | (8 << 24) | (11 << 19) | (13 << 12) | (7 << 7) | 5)
    t:expect(t.expr(cfg):shiftctrl()):fmt(hex8)
        :eq(0x800f0000 | (10 << 25) | (9 << 20))
    t:expect(t.expr(cfg):pinctrl()):fmt(hex8)
        :eq((1 << 29) | (4 << 26) | (2 << 20) | (5 << 15) | (6 << 10)
            | (3 << 5) | 1)
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
        t:expect(t.expr(inst):get_index()):eq(i)
        t:expect(t.expr(inst):regs()):eq(addressmap[('PIO%s_BASE'):format(i)])
    end
end

local function setup(t, prog, clkdiv, irq_mask)
    -- Claim a state machine.
    local inst = pio[1]
    inst:interrupt_clear_mask((1 << pio.NUM_IRQS) - 1)
    local sm = inst:sm(2)
    sm:claim();
    t:cleanup(function()
        sm:unclaim()
        t:expect(t.expr(sm):is_claimed()):eq(false)
    end)
    t:expect(t.expr(sm):index()):eq(2)
    t:expect(t.expr(sm):is_claimed()):eq(true)

    -- Enable IRQs if non-blocking behavior is enabled.
    if not thread.blocking() then
        local mask = (1 << (pio.pis_sm0_tx_fifo_not_full + sm:index()))
                     | (1 << (pio.pis_sm0_rx_fifo_not_empty + sm:index()))
                     | ((irq_mask or 0) << pio.pis_interrupt0)
        inst:enable_irq(mask)
        t:cleanup(function() inst:enable_irq(mask, false) end)
    end

    -- Load the program.
    t:assert(t.expr(inst):can_add_program(prog)):eq(true)
    local off = inst:add_program(prog)
    t:expect(off):label("offset"):eq(32 - #prog)
    t:cleanup(function() inst:remove_program(prog, off) end)

    -- Configure and enable the state machine.
    local cfg = pio.get_default_sm_config()
        :set_wrap(prog.wrap_target + off, prog.wrap + off)
    cfg:set_clkdiv_int_frac(clkdiv)
    sm:init(prog.labels.start + off, cfg)
    t:cleanup(function() sm:set_enabled(false) end)
    sm:set_enabled(true)
    return inst, sm
end

local pio_timer = {
    0xe040, 0xa04a, 0x80a0, 0xa027, 0x0044, 0xa0ca, 0x8020, 0x0082,
    labels = {start = 0}, wrap_target = 2, wrap = 7,
}

function test_put_BNB(t)
    local inst, sm = setup(t, pio_timer, 250)
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
    local inst, sm = setup(t, pio_timer, 250)
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
    0xca20, 0xca21, 0xca22, 0xca23,
    labels = {start = 0}, wrap_target = 0, wrap = 3,
}

function test_irqs_BNB(t)
    local inst, sm = setup(t, pio_irqs, 2500, 0xf)
    local log = list()
    for i = 1, 10 do
        local pending = inst:wait_irq(0xf)
        log:append(pending)
        for j = 0, pio.NUM_SYS_IRQS - 1 do
            t:expect(t.expr(inst):interrupt_get(j))
                :eq((pending & (1 << j) ~= 0))
        end
        inst:interrupt_clear_mask(pending)
    end
    t:expect(log):label("log"):eq{1, 2, 4, 8, 1, 2, 4, 8, 1, 2}
end
