_ENV = mlua.Module(...)

local base = require 'hardware.base'
local vreg = require 'hardware.vreg'
local addressmap = require 'hardware.regs.addressmap'
local vacr = require 'hardware.regs.vreg_and_chip_reset'

local function get_voltage()
    return (base.read32(addressmap.VREG_AND_CHIP_RESET_BASE
                       + vacr.VREG_AND_CHIP_RESET_VREG_OFFSET)
            & vacr.VREG_AND_CHIP_RESET_VREG_VSEL_BITS)
           >> vacr.VREG_AND_CHIP_RESET_VREG_VSEL_LSB
end

function test_set_voltage(t)
    t:expect(get_voltage()):label("voltage"):eq(vreg.VOLTAGE_DEFAULT)
    vreg.set_voltage(vreg.VOLTAGE_1_20)
    t:expect(get_voltage()):label("voltage"):eq(vreg.VOLTAGE_1_20)
    vreg.set_voltage(vreg.VOLTAGE_DEFAULT)
    t:expect(get_voltage()):label("voltage"):eq(vreg.VOLTAGE_DEFAULT)
end
