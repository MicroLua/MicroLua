-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local base = require 'hardware.base'
local i2c = require 'hardware.i2c'
local addressmap = require 'hardware.regs.addressmap'
local regs = require 'hardware.regs.i2c'
local config = require 'mlua.config'
local testing_i2c = require 'mlua.testing.i2c'
local thread = require 'mlua.thread'
local multicore = require 'pico.multicore'

local module_name = ...

function test_index_base(t)
    for i = 0, i2c.NUM - 1 do
        local inst = i2c[i]
        t:expect(t.expr(inst):hw_index()):eq(i)
        t:expect(t.expr(inst):regs()):eq(addressmap[('I2C%s_BASE'):format(i)])
    end
end

local function run_slave(inst, on_receive, on_request)
    local i2c_base = inst:regs()
    while true do
        while inst:get_read_available() > 0 do
            local dc = inst:read_data_cmd()
            on_receive(inst, dc & 0xff, dc > 0xff)
        end
        local intr_stat = base.read32(i2c_base + regs.IC_INTR_STAT_OFFSET)
        if intr_stat & regs.IC_INTR_STAT_R_RD_REQ_BITS ~= 0 then
            base.read32(i2c_base + regs.IC_CLR_RD_REQ_OFFSET)
            on_request(inst)
        end
        thread.yield()
    end
end

function test_master_BNB(t)
    local slave_addr = 0x17
    local master, slave = testing_i2c.set_up(
        t, 1000000, config.I2C_MASTER_SDA, config.I2C_MASTER_SCL,
        config.I2C_SLAVE_SDA, config.I2C_SLAVE_SCL)
    slave:set_slave_mode(true, slave_addr)
    base.write32(slave:regs() + regs.IC_INTR_MASK_OFFSET,
                 regs.IC_INTR_MASK_M_RD_REQ_BITS)

    -- Start the slave on core 1 if blocking is enabled, or as a thread.
    if thread.blocking() then
        multicore.launch_core1(module_name, 'core1_slave')
        t:cleanup(multicore.reset_core1)
    else
        local th = thread.start(function()
            run_slave(slave, testing_i2c.mem_slave(32))
        end)
        t:cleanup(function() th:kill() end)
        master:enable_irq()
        t:cleanup(function() master:enable_irq(false) end)
    end

    -- Write some data, then read it back.
    local data = 'abcdefghijklmnopqrstuvwxyz012345'
    t:expect(t.expr(master):write_blocking(slave_addr, '\x00' .. data, false))
        :eq(1 + #data)
    t:expect(t.expr(master):write_blocking(slave_addr, '\x07', true)):eq(1)
    t:expect(t.expr(master):read_blocking(slave_addr, 25, true))
        :eq('hijklmnopqrstuvwxyz012345')
    t:expect(t.expr(master):read_blocking(slave_addr, 7, false)):eq('abcdefg')

    -- Perform a large read.
    t:expect(t.expr(master):write_blocking(slave_addr, '\x00', true)):eq(1)
    t:expect(t.expr(master):read_blocking(slave_addr, 10 * #data, false))
        :eq(data:rep(10))
end

function core1_slave()
    multicore.set_shutdown_handler()
    local slave = testing_i2c.find_instance(config.I2C_SLAVE_SDA)
    run_slave(slave, testing_i2c.mem_slave(32))
end
