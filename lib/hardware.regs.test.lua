_ENV = mlua.Module(...)

local addressmap = require 'hardware.regs.addressmap'
local intctrl = require 'hardware.regs.intctrl'
local package = require 'package'

function test_require_all(t)
    for name in pairs(package.preload) do
        if name:match('^hardware%.regs%.') then require(name) end
    end
end

function test_symbols(t)
    t:expect(t:expr(addressmap).REG_ALIAS_CLR_BITS):eq(3 << 12)
    t:expect(t:expr(addressmap).IO_BANK0_BASE):eq(0x40014000)
    t:expect(t:expr(addressmap).UART0_BASE):eq(0x40034000)
    t:expect(t:expr(intctrl).IO_IRQ_BANK0):eq(13)
    t:expect(t:expr(intctrl).UART0_IRQ):eq(20)
end