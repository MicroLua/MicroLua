-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local gpio = require 'hardware.gpio'
local regs = require 'hardware.regs.io_bank0'
local irq = require 'hardware.irq'
local config = require 'mlua.config'
local list = require 'mlua.list'
local thread = require 'mlua.thread'
local util = require 'mlua.util'
local string = require 'string'

local pin1 = config.GPIO_PIN1
local pin2 = config.GPIO_PIN2
local all_events = gpio.IRQ_LEVEL_LOW | gpio.IRQ_LEVEL_HIGH
                   | gpio.IRQ_EDGE_FALL | gpio.IRQ_EDGE_RISE

local function config_pin(t)
    gpio.init(pin1)
    t:cleanup(function()
        gpio.deinit(pin1)
        gpio.set_pulls(pin1, false, true)
        gpio.set_irqover(pin1, gpio.OVERRIDE_NORMAL)
        gpio.set_outover(pin1, gpio.OVERRIDE_NORMAL)
        gpio.set_inover(pin1, gpio.OVERRIDE_NORMAL)
        gpio.set_oeover(pin1, gpio.OVERRIDE_NORMAL)
        gpio.set_input_hysteresis_enabled(pin1, true)
        gpio.set_slew_rate(pin1, gpio.SLEW_RATE_SLOW)
        gpio.set_drive_strength(pin1, gpio.DRIVE_STRENGTH_4MA)
        gpio.set_irq_enabled(pin1, all_events, false)
        gpio.set_dir(pin1, gpio.IN)
    end)
end

function test_function(t)
    config_pin(t)
    t:expect(t.expr(gpio).get_function(pin1)):eq(gpio.FUNC_SIO)
    gpio.set_function(pin1, gpio.FUNC_NULL)
    t:expect(t.expr(gpio).get_function(pin1)):eq(gpio.FUNC_NULL)
end

function test_pulls(t)
    config_pin(t)
    t:expect(t.expr(gpio).is_pulled_up(pin1)):eq(false)
    t:expect(t.expr(gpio).is_pulled_down(pin1)):eq(true)
    gpio.set_pulls(pin1, true, false)
    t:expect(t.expr(gpio).is_pulled_up(pin1)):eq(true)
    t:expect(t.expr(gpio).is_pulled_down(pin1)):eq(false)
    gpio.pull_down(pin1)
    t:expect(t.expr(gpio).is_pulled_up(pin1)):eq(false)
    t:expect(t.expr(gpio).is_pulled_down(pin1)):eq(true)
    gpio.pull_up(pin1)
    t:expect(t.expr(gpio).is_pulled_up(pin1)):eq(true)
    t:expect(t.expr(gpio).is_pulled_down(pin1)):eq(false)
    gpio.disable_pulls(pin1)
    t:expect(t.expr(gpio).is_pulled_up(pin1)):eq(false)
    t:expect(t.expr(gpio).is_pulled_down(pin1)):eq(false)
end

function test_overrides(t)
    config_pin(t)
    gpio.put(pin1, 1)
    gpio.set_dir(pin1, gpio.OUT)

    -- IRQ override
    irq.clear(irq.IO_IRQ_BANK0)
    gpio.set_irq_enabled(pin1, gpio.IRQ_LEVEL_LOW, true)
    t:expect(t.expr.irq.is_pending(irq.IO_IRQ_BANK0)):eq(false)
    gpio.set_irqover(pin1, gpio.OVERRIDE_INVERT)
    t:expect(t.expr.irq.is_pending(irq.IO_IRQ_BANK0)):eq(true)
    gpio.set_irqover(pin1, gpio.OVERRIDE_NORMAL)

    -- Output override
    t:expect(t.expr(gpio).get(pin1)):eq(true)
    gpio.set_outover(pin1, gpio.OVERRIDE_INVERT)
    t:expect(t.expr(gpio).get(pin1)):eq(false)
    gpio.set_outover(pin1, gpio.OVERRIDE_NORMAL)

    -- Input override
    t:expect(t.expr(gpio).get(pin1)):eq(true)
    gpio.set_inover(pin1, gpio.OVERRIDE_LOW)
    t:expect(t.expr(gpio).get(pin1)):eq(false)
    gpio.set_inover(pin1, gpio.OVERRIDE_NORMAL)

    -- Output enable override
    t:expect(t.expr(gpio).get(pin1)):eq(true)
    gpio.set_oeover(pin1, gpio.OVERRIDE_LOW)
    t:expect(t.expr(gpio).get(pin1)):eq(false)
    gpio.set_oeover(pin1, gpio.OVERRIDE_NORMAL)
end

function test_input_hysteresis(t)
    config_pin(t)
    t:expect(t.expr(gpio).is_input_hysteresis_enabled(pin1)):eq(true)
    gpio.set_input_hysteresis_enabled(pin1, false)
    t:expect(t.expr(gpio).is_input_hysteresis_enabled(pin1)):eq(false)
end

function test_slew_rate(t)
    config_pin(t)
    t:expect(t.expr(gpio).get_slew_rate(pin1)):eq(gpio.SLEW_RATE_SLOW)
    gpio.set_slew_rate(pin1, gpio.SLEW_RATE_FAST)
    t:expect(t.expr(gpio).get_slew_rate(pin1)):eq(gpio.SLEW_RATE_FAST)
end

function test_drive_strength(t)
    config_pin(t)
    t:expect(t.expr(gpio).get_drive_strength(pin1)):eq(gpio.DRIVE_STRENGTH_4MA)
    gpio.set_drive_strength(pin1, gpio.DRIVE_STRENGTH_8MA)
    t:expect(t.expr(gpio).get_drive_strength(pin1)):eq(gpio.DRIVE_STRENGTH_8MA)
end

function test_dir(t)
    config_pin(t)
    t:expect(t.expr(gpio).is_dir_out(pin1)):eq(false)
    t:expect(t.expr(gpio).get_dir(pin1)):eq(gpio.IN)
    gpio.set_dir_out_masked(1 << pin1)
    t:expect(t.expr(gpio).is_dir_out(pin1)):eq(true)
    t:expect(t.expr(gpio).get_dir(pin1)):eq(gpio.OUT)
    gpio.set_dir_in_masked(1 << pin1)
    t:expect(t.expr(gpio).is_dir_out(pin1)):eq(false)
    t:expect(t.expr(gpio).get_dir(pin1)):eq(gpio.IN)
    gpio.set_dir_masked(1 << pin1, 1 << pin1)
    t:expect(t.expr(gpio).is_dir_out(pin1)):eq(true)
    t:expect(t.expr(gpio).get_dir(pin1)):eq(gpio.OUT)
    gpio.set_dir_masked(1 << pin1, 0)
    t:expect(t.expr(gpio).is_dir_out(pin1)):eq(false)
    t:expect(t.expr(gpio).get_dir(pin1)):eq(gpio.IN)
    local v = 0
    for i = 0, 31 do v = v | (gpio.is_dir_out(pin1) and (1 << i) or 0) end
    gpio.set_dir_all_bits(v | (1 << pin1))
    t:expect(t.expr(gpio).is_dir_out(pin1)):eq(true)
    t:expect(t.expr(gpio).get_dir(pin1)):eq(gpio.OUT)
    gpio.set_dir(pin1, gpio.IN)
    t:expect(t.expr(gpio).is_dir_out(pin1)):eq(false)
    t:expect(t.expr(gpio).get_dir(pin1)):eq(gpio.IN)
end

function test_init(t)
    t:cleanup(function() gpio.deinit(pin1) end)
    t:expect(t.expr(gpio).get_function(pin1)):eq(gpio.FUNC_NULL)
    gpio.init(pin1)
    t:expect(t.expr(gpio).get_function(pin1)):eq(gpio.FUNC_SIO)
    gpio.deinit(pin1)
    t:expect(t.expr(gpio).get_function(pin1)):eq(gpio.FUNC_NULL)
    gpio.init_mask(1 << pin1)
    t:expect(t.expr(gpio).get_function(pin1)):eq(gpio.FUNC_SIO)
end

function test_driving(t)
    config_pin(t)
    gpio.put(pin1, 0)
    gpio.set_dir(pin1, gpio.OUT)

    gpio.put(pin1, 1)
    t:expect(t.expr(gpio).get_out_level(pin1)):eq(true)
    t:expect(t.expr(gpio).get(pin1)):eq(true)
    t:expect(gpio.get_all() & (1 << pin1)):label("gpio.get_all()")
        :eq((1 << pin1))
    t:expect(t.expr(gpio).get_pad(pin1)):eq(true)
    t:expect(gpio.get_status(pin1) & regs.GPIO0_STATUS_INFROMPAD_BITS)
        :label("pad"):eq(regs.GPIO0_STATUS_INFROMPAD_BITS)

    gpio.set_input_enabled(pin1, false)
    t:expect(t.expr(gpio).get(pin1)):eq(false)
    t:expect(gpio.get_all() & (1 << pin1)):label("gpio.get_all()"):eq(0)
    gpio.set_input_enabled(pin1, true)

    gpio.put(pin1, 0)
    t:expect(t.expr(gpio).get_out_level(pin1)):eq(false)
    t:expect(t.expr(gpio).get(pin1)):eq(false)
    t:expect(gpio.get_all() & (1 << pin1)):label("gpio.get_all()"):eq(0)
    t:expect(t.expr(gpio).get_pad(pin1)):eq(false)
    t:expect(gpio.get_status(pin1) & regs.GPIO0_STATUS_INFROMPAD_BITS)
        :label("pad"):eq(0)

    gpio.set_mask(1 << pin1)
    t:expect(t.expr(gpio).get_out_level(pin1)):eq(true)
    gpio.clr_mask(1 << pin1)
    t:expect(t.expr(gpio).get_out_level(pin1)):eq(false)
    gpio.xor_mask(1 << pin1)
    t:expect(t.expr(gpio).get_out_level(pin1)):eq(true)
    gpio.put_masked(1 << pin1, 0)
    t:expect(t.expr(gpio).get_out_level(pin1)):eq(false)
    local v = 0
    for i = 0, 31 do v = v | (gpio.get_out_level(pin1) and (1 << i) or 0) end
    gpio.put_all(v | (1 << pin1))
    t:expect(t.expr(gpio).get_out_level(pin1)):eq(true)
end

function test_irq_events(t)
    config_pin(t)
    gpio.put(pin1, 0)
    gpio.set_dir(pin1, gpio.OUT)
    gpio.set_irq_enabled(pin1,
        gpio.IRQ_LEVEL_HIGH | gpio.IRQ_EDGE_FALL | gpio.IRQ_EDGE_RISE, true)

    t:expect(t.expr(gpio).get_irq_event_mask(pin1)):eq(0)
    gpio.put(pin1, 1)
    t:expect(t.expr(gpio).get_irq_event_mask(pin1))
        :eq(gpio.IRQ_LEVEL_HIGH | gpio.IRQ_EDGE_RISE)
    gpio.acknowledge_irq(pin1, all_events)
    t:expect(t.expr(gpio).get_irq_event_mask(pin1)):eq(gpio.IRQ_LEVEL_HIGH)
    gpio.put(pin1, 0)
    t:expect(t.expr(gpio).get_irq_event_mask(pin1)):eq(gpio.IRQ_EDGE_FALL)
    gpio.acknowledge_irq(pin1, all_events)
    t:expect(t.expr(gpio).get_irq_event_mask(pin1)):eq(0)
end

function test_set_irq_callback(t)
    config_pin(t)
    gpio.put(pin1, 0)
    gpio.set_dir(pin1, gpio.OUT)

    -- Add a GPIO callback.
    local events = {'LEVEL_LOW', 'LEVEL_HIGH', 'EDGE_FALL', 'EDGE_RISE'}
    local log = {}
    local mask = gpio.IRQ_LEVEL_HIGH | gpio.IRQ_EDGE_FALL | gpio.IRQ_EDGE_RISE
    gpio.set_irq_enabled_with_callback(pin1, mask, true, function(p, em)
        local names = list()
        for _, e in ipairs(events) do
            if em & gpio[('IRQ_%s'):format(e)] ~= 0 then names:append(e) end
        end
        log[p] = (log[p] or '') .. ('(%s) '):format(names:concat(' '))
    end)
    t:cleanup(function()
        irq.set_enabled(irq.IO_IRQ_BANK0, false)
        gpio.set_irq_callback(nil)
    end)

    -- Configure a second pin1.
    gpio.init(pin2)
    gpio.set_dir(pin2, gpio.OUT)
    t:cleanup(function()
        gpio.deinit(pin2)
        gpio.set_irq_enabled(pin2, all_events, false)
    end)
    local mask2 = gpio.IRQ_EDGE_FALL | gpio.IRQ_EDGE_RISE
    gpio.set_irq_enabled(pin2, mask2, true)

    -- Trigger events and let the callback record.
    gpio.put(pin1, 1)
    thread.yield()
    gpio.put(pin2, 1)
    thread.yield()
    gpio.put(pin1, 0)
    gpio.put(pin2, 0)
    thread.yield()

    local want = {
        [pin1] = '(LEVEL_HIGH EDGE_RISE) (LEVEL_HIGH) (EDGE_FALL) ',
        [pin2] = '(EDGE_RISE) (EDGE_FALL) ',
    }
    t:expect(log):label("log"):eq(want, util.table_eq)
end
