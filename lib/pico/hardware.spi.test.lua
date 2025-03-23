-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local addressmap = require 'hardware.regs.addressmap'
local spi = require 'hardware.spi'
local thread = require 'mlua.thread'
local string = require 'string'

function test_index_base(t)
    for i = 0, spi.NUM - 1 do
        local inst = spi[i]
        t:expect(t.expr(inst):get_index()):eq(i)
        t:expect(t.expr(inst):regs()):eq(addressmap[('SPI%s_BASE'):format(i)])
    end
end

local function setup(t, bits)
    local inst = spi[0]
    local baud = 24000000
    t:expect(t.expr(inst):init(baud)):close_to_rel(baud, 0.001)
    t:cleanup(function() inst:deinit() end)
    inst:set_format(bits, spi.CPOL_0, spi.CPHA_0);
    inst:enable_loopback(true)
    if not thread.blocking() then inst:enable_irq() end
    return inst
end

function test_configuration(t)
    local inst = setup(t, 8)
    local baud = 12000000
    t:expect(t.expr(inst):set_baudrate(baud)):close_to_rel(baud, 0.001)
        :eq(inst:get_baudrate())
    inst:set_format(6, spi.CPOL_1, spi.CPHA_1);
    inst:set_slave(false)
    t:expect(t.expr(inst):is_writable()):eq(true)
    t:expect(t.expr(inst):is_readable()):eq(false)
    t:expect(t.expr(inst):is_busy()):eq(false)
    t:expect(t.expr(inst):write_read_blocking('\xa8\xb7\xc6\xd5\xe4\xf3'))
        :eq('\x28\x37\x06\x15\x24\x33')
end

function test_write_read_blocking_8bit_BNB(t)
    local inst = setup(t, 8)
    local data = 'abcdefghijklmnopqrstuvwxyz0'
    t:expect(t.expr(inst):write_read_blocking(data)):eq(data)
    inst:write_blocking(data)
    t:expect(t.expr(inst):read_blocking(0x42, 13)):eq(('\x42'):rep(13))
end

function test_write_read_blocking_12bit_BNB(t)
    local inst = setup(t, 12)
    local data = '\x01\x23\x45\x67\x89\xab\xcd\xef\xf0\xe1\xd2\xc3\xb4\xa5'
                 .. '\x96\x87\x78\x69\x5a\x4b\x3c\x2d\x1e\x0f'
    t:expect(t.expr(inst):write_read_blocking(data))
        :eq('\x01\x03\x45\x07\x89\x0b\xcd\x0f\xf0\x01\xd2\x03\xb4\x05'
            .. '\x96\x07\x78\x09\x5a\x0b\x3c\x0d\x1e\x0f')
    inst:write_blocking(data)
    t:expect(t.expr(inst):read_blocking(0xf123, 17)):eq(('\x23\x01'):rep(17))
end
