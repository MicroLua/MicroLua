-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local addressmap = require 'hardware.regs.addressmap'
local dreq = require 'hardware.regs.dreq'
local intctrl = require 'hardware.regs.intctrl'
local vreg_and_chip_reset = require 'hardware.regs.vreg_and_chip_reset'
local package = require 'package'

function test_require_all(t)
    for name in pairs(package.preload) do
        if name:match('^hardware%.regs%.') then require(name) end
    end
end

function test_symbols(t)
    t:expect(t.expr(addressmap).REG_ALIAS_CLR_BITS):eq(3 << 12)
    t:expect(t.expr(addressmap).IO_BANK0_BASE):eq(pointer(0x40014000))
    t:expect(t.expr(addressmap).UART0_BASE):eq(pointer(0x40034000))
    t:expect(t.expr(addressmap).SRAM_END):eq(pointer(0x20042000))
    t:expect(t.expr(dreq).UART0_TX):eq(20)
    t:expect(t.expr(dreq).COUNT):eq(64)
    t:expect(t.expr(intctrl).IO_IRQ_BANK0):eq(13)
    t:expect(t.expr(intctrl).UART0_IRQ):eq(20)
    t:expect(t.expr(intctrl).IRQ_COUNT):eq(26)
    t:expect(t.expr(vreg_and_chip_reset).VREG_VSEL_LSB):eq(4)
    t:expect(t.expr(vreg_and_chip_reset).CHIP_RESET_PSM_RESTART_FLAG_BITS)
        :eq(0x01000000)
end
