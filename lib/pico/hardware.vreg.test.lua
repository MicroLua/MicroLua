-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local base = require 'hardware.base'
local vreg = require 'hardware.vreg'
local addressmap = require 'hardware.regs.addressmap'
local vreg_and_chip_reset = require 'hardware.regs.vreg_and_chip_reset'

local function get_voltage()
    return (base.read32(addressmap.VREG_AND_CHIP_RESET_BASE
                        + vreg_and_chip_reset.VREG_OFFSET)
            & vreg_and_chip_reset.VREG_VSEL_BITS)
           >> vreg_and_chip_reset.VREG_VSEL_LSB
end

function test_set_voltage(t)
    t:expect(get_voltage()):label("voltage"):eq(vreg.VOLTAGE_DEFAULT)
    vreg.set_voltage(vreg.VOLTAGE_1_20)
    t:expect(get_voltage()):label("voltage"):eq(vreg.VOLTAGE_1_20)
    vreg.set_voltage(vreg.VOLTAGE_DEFAULT)
    t:expect(get_voltage()):label("voltage"):eq(vreg.VOLTAGE_DEFAULT)
end
