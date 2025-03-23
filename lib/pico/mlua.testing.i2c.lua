-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local gpio = require 'hardware.gpio'
local i2c = require 'hardware.i2c'
local list = require 'mlua.list'
local time = require 'mlua.time'

pins = {
    [0] = {sda = {0, 4, 8, 12, 16, 20, 24, 28},
           scl = {1, 5, 9, 13, 17, 21, 25, 29}},
    [1] = {sda = {2, 6, 10, 14, 18, 22, 26},
           scl = {3, 7, 11, 15, 19, 23, 27}},
}

-- Returns the I2C instance for the given pin.
function find_instance(pin)
    for i, ps in pairs(pins) do
        if list.find(ps.sda, pin) or list.find(ps.scl, pin) then
            return i2c[i]
        end
    end
end

-- Set up one or two I2C instances, and in the latter case, check that they are
-- on the same bus.
function set_up(t, baud, sda0, scl0, sda1, scl1)
    local inst0, inst1 = find_instance(sda0), nil
    t:assert(inst0, "No I2C instance found for pin %s", sda0)
    t:assert(find_instance(scl0) == inst0,
             "Pins %s and %s are on distinct I2C instances", sda0, scl0)
    t:cleanup(function()
        gpio.deinit(sda0)
        gpio.deinit(scl0)
        inst0:deinit()
    end)
    local pins = list()
    for _, pin in ipairs{sda0, scl0} do
        gpio.init(pin)
        gpio.pull_up(pin)
        pins:append(pin)
    end

    if sda1 and scl1 then
        inst1 = find_instance(sda1)
        t:assert(inst1, "No I2C instance found for pin %s", sda1)
        t:assert(find_instance(scl1) == inst1,
                 "Pins %s and %s are on distinct I2C instances", sda1, scl1)
        t:cleanup(function()
            gpio.deinit(sda1)
            gpio.deinit(scl1)
            inst1:deinit()
        end)
        for _, pin in ipairs{sda1, scl1} do
            gpio.init(pin)
            gpio.pull_up(pin)
            pins:append(pin)
        end
    end

    for _, pin in pins:ipairs() do
        t:assert(gpio.get_pad(pin), "Pin %s is forced low", pin)
    end
    if sda1 and scl1 then
        for _, pp in ipairs{{sda0, sda1}, {scl0, scl1}} do
            local mpin, spin = pp[1], pp[2]
            local done<close> = function()
                gpio.set_oeover(mpin, gpio.OVERRIDE_NORMAL)
            end
            gpio.set_oeover(mpin, gpio.OVERRIDE_HIGH)
            t:assert(not gpio.get_pad(spin),
                     "Pin %s must be connected to pin %s", mpin, spin)
        end
    end

    t:expect(t.expr(inst0):init(baud)):close_to_rel(baud, 0.01)
    gpio.set_function(sda0, gpio.FUNC_I2C)
    gpio.set_function(scl0, gpio.FUNC_I2C)

    if sda1 and scl1 then
        t:expect(t.expr(inst1):init(baud)):close_to_rel(baud, 0.01)
        gpio.set_function(sda1, gpio.FUNC_I2C)
        gpio.set_function(scl1, gpio.FUNC_I2C)
    end

    return inst0, inst1
end

-- Return handlers for a slave emulating a memory with the given size.
function mem_slave(size)
    local mem, addr = {}, nil
    return function(inst, data, first)
        if first then
            addr = data % size
        else
            mem[addr] = data
            addr = (addr + 1) % size
        end
    end, function(inst)
        inst:write_byte_raw(mem[addr] or 0)
        addr = (addr + 1) % size
    end
end
