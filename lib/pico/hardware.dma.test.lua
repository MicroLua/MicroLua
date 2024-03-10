-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local dma = require 'hardware.dma'
local addressmap = require 'hardware.regs.addressmap'
local regs = require 'hardware.regs.dma'
local dreq = require 'hardware.regs.dreq'
local mem = require 'mlua.mem'
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
    t:expect(t:expr(cfg):ctrl()):fmt(hex8)
        :eq(0x00e00422 | (dreq.ADC << 15) | (11 << 11) | (3 << 6))
end

function test_regs(t)
    t:expect(t:expr(dma).regs()):fmt(hex8):eq(addressmap.DMA_BASE)
    for i = 0, dma.NUM_CHANNELS - 1 do
        t:expect(t:expr(dma).regs(i)):fmt(hex8)
            :eq(addressmap.DMA_BASE + i * regs.CH1_READ_ADDR_OFFSET)
    end
end

function test_mem_to_mem(t)
    t:cleanup(function() dma.sniffer_disable() end)
    local ch = dma.claim_unused_channel()
    t:cleanup(function()
        dma.channel_cleanup(ch)
        dma.channel_unclaim(ch)
        t:expect(t:expr(dma).channel_is_claimed(ch)):eq(false)
    end)
    t:expect(t:expr(dma).channel_is_claimed(ch)):eq(true)
    local cfg = dma.channel_get_default_config(ch)
        :set_read_increment(true)
        :set_write_increment(true)
        :set_transfer_data_size(dma.SIZE_8)
        :set_sniff_enable(true)

    local src = "The quick brown fox jumps over the lazy dog"
    local dst = mem.alloc(#src):fill(0)
    -- CRC-32 computation using the CRC32 preset on
    -- <http://www.sunshine2k.de/coding/javascript/crc/crc_js.html>.
    dma.sniffer_enable(ch, dma.SNIFF_CRC32_REV, false)
    dma.sniffer_set_data_accumulator(0xffffffff)
    dma.sniffer_set_output_invert_enabled(true)
    dma.sniffer_set_output_reverse_enabled(true)
    dma.channel_configure(ch, cfg, dst, src, #src, true)
    while dma.channel_busy(ch) do end
    t:expect(t:expr(dst):read()):eq(src)
    t:expect(t:expr(dma).sniffer_get_data_accumulator()):fmt(hex8)
        :eq(0x414fa339)
end
