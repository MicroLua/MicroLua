-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local dma = require 'hardware.dma'
local addressmap = require 'hardware.regs.addressmap'
local regs = require 'hardware.regs.dma'
local dreq = require 'hardware.regs.dreq'
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
