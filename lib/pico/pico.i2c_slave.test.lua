-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local config = require 'mlua.config'
local testing_i2c = require 'mlua.testing.i2c'
local i2c_slave = require 'pico.i2c_slave'

function test_slave(t)
    local slave_addr = 0x17
    local master, slave = testing_i2c.set_up(
        t, 1000000, config.I2C_MASTER_SDA, config.I2C_MASTER_SCL,
        config.I2C_SLAVE_SDA, config.I2C_SLAVE_SCL)
    master:enable_irq()
    t:cleanup(function() master:enable_irq(false) end)
    slave:enable_irq()
    t:cleanup(function() slave:enable_irq(false) end)
    local th = i2c_slave.run(slave, slave_addr, testing_i2c.mem_slave(32))
    t:cleanup(function() th:kill() end)

    -- Write some data, then read it back.
    local data = 'abcdefghijklmnopqrstuvwxyz012345'
    t:expect(t.expr(master):write_blocking(slave_addr, '\x00' .. data, false))
        :eq(1 + #data)
    t:expect(t.expr(master):write_blocking(slave_addr, '\x07', true)):eq(1)
    t:expect(t.expr(master):read_blocking(slave_addr, 25, true))
        :eq('hijklmnopqrstuvwxyz012345')
    t:expect(t.expr(master):read_blocking(slave_addr, 7, false)):eq('abcdefg')
end
