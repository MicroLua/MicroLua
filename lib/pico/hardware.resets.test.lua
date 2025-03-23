-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local base = require 'hardware.base'
local adc = require 'hardware.regs.adc'
local addressmap = require 'hardware.regs.addressmap'
local regs = require 'hardware.regs.resets'
local resets = require 'hardware.resets'

function test_reset(t)
    local div = addressmap.ADC_BASE + adc.DIV_OFFSET
    base.write32(div, 1234)
    t:expect(t.expr(base).read32(div)):eq(1234)

    local bits = regs.RESET_ADC_BITS
    resets.reset_block(bits)
    resets.unreset_block_wait(bits)

    t:expect(t.expr(base).read32(div)):eq(adc.DIV_RESET)
end
