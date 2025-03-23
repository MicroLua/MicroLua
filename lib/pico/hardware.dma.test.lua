-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local dma = require 'hardware.dma'
local addressmap = require 'hardware.regs.addressmap'
local regs = require 'hardware.regs.dma'
local dreq = require 'hardware.regs.dreq'
local mem = require 'mlua.mem'
local thread = require 'mlua.thread'
local time = require 'mlua.time'
local string = require 'string'

local function hex8(v) return ('0x%08x'):format(v) end

function test_config(t)
    local cfg = dma.channel_get_default_config(5)
        :set_read_increment(false)
        :set_write_increment(true)
        :set_dreq(dreq.ADC)
        :set_chain_to(11)
        :set_transfer_data_size(dma.SIZE_8)
        :set_ring(true, 3)
        :set_bswap(true)
        :set_irq_quiet(true)
        :set_high_priority(true)
        :set_enable(false)
        :set_sniff_enable(true)
    t:expect(t.expr(cfg):ctrl()):fmt(hex8)
        :eq(0x00e00422 | (dreq.ADC << 15) | (11 << 11) | (3 << 6))
end

function test_regs(t)
    t:expect(t.expr(dma).regs()):eq(addressmap.DMA_BASE)
    for i = 0, dma.NUM_CHANNELS - 1 do
        t:expect(t.expr(dma).regs(i))
            :eq(addressmap.DMA_BASE + i * regs.CH1_READ_ADDR_OFFSET)
    end
end

function test_mem_to_mem_BNB(t)
    t:cleanup(function() dma.sniffer_disable() end)

    -- Claim a DMA channel.
    local ch = dma.claim_unused_channel()
    t:cleanup(function()
        dma.channel_cleanup(ch)
        dma.channel_unclaim(ch)
        t:expect(t.expr(dma).channel_is_claimed(ch)):eq(false)
    end)
    t:expect(t.expr(dma).channel_is_claimed(ch)):eq(true)

    -- Claim a DMA timer and configure it.
    local timer = dma.claim_unused_timer()
    t:cleanup(function()
        dma.timer_unclaim(timer)
        t:expect(t.expr(dma).timer_is_claimed(timer)):eq(false)
    end)
    t:expect(t.expr(dma).timer_is_claimed(timer)):eq(true)
    dma.timer_set_fraction(timer, 1, 50 * 250)

    -- Enable the DMA IRQ if non-blocking behavior is enabled.
    if not thread.blocking() then
        dma.enable_irq(1 << ch)
        t:cleanup(function() dma.enable_irq(1 << ch, false) end)
    end

    -- Prepare the channel configuration.
    local cfg = dma.channel_get_default_config(ch)
        :set_read_increment(true)
        :set_write_increment(true)
        :set_dreq(dma.get_timer_dreq(timer))
        :set_transfer_data_size(dma.SIZE_8)
        :set_sniff_enable(true)

    -- Perform transfers. The CRC-32 computation corresponds to the CRC32 preset
    -- on <http://www.sunshine2k.de/coding/javascript/crc/crc_js.html>.
    local src = "The quick brown fox jumps over the lazy dog"
    local dst = mem.alloc(#src)
    dma.sniffer_enable(ch, dma.SNIFF_CRC32_REV, false)
    dma.sniffer_set_output_invert_enabled(true)
    dma.sniffer_set_output_reverse_enabled(true)
    for i = 1, 3 do
        mem.fill(dst, 0)
        dma.sniffer_set_data_accumulator(0xffffffff)
        t:expect(t.expr(dma).channel_is_busy(ch)):eq(false)
        local t1 = time.ticks()
        dma.channel_configure(ch, cfg, dst, src, #src, true)
        t:expect(t.expr(dma).channel_is_busy(ch)):eq(true)
        t:expect(t.expr(dma).channel_get_irq_status(ch)):eq(false)
        local m = dma.wait_irq(1 << ch)
        local t2 = time.ticks()
        t:expect(m):label("pending IRQs"):eq(1 << ch)
        t:expect(t.expr(dma).channel_is_busy(ch)):eq(false)
        t:expect(t.expr(dma).channel_get_irq_status(ch)):eq(false)
        t:expect(t.expr(mem).read(dst)):eq(src)
        t:expect(t.expr(dma).sniffer_get_data_accumulator()):fmt(hex8)
            :eq(0x414fa339)
        t:expect(t2 - t1):label("duration"):gte(50 * #src - 20)
    end
end
