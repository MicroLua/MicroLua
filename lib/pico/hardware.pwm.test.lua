-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local gpio = require 'hardware.gpio'
local pwm = require 'hardware.pwm'
local addressmap = require 'hardware.regs.addressmap'
local regs = require 'hardware.regs.pwm'
local config = require 'mlua.config'
local thread = require 'mlua.thread'
local util = require 'mlua.util'
local pico = require 'pico'

local pin = config.GPIO_PIN1
local slice = pwm.gpio_to_slice_num(pin)
local chan = pwm.gpio_to_channel(pin)

local function hex8(v) return ('0x%08x'):format(v) end

function test_introspect(t)
    t:expect(t.expr(pwm).gpio_to_slice_num(13)):eq(6)
    t:expect(t.expr(pwm).gpio_to_channel(13)):eq(pwm.CHAN_B)
    t:expect(t.expr(pwm).regs()):eq(addressmap.PWM_BASE)
    for i = 0, pwm.NUM_SLICES - 1 do
        t:expect(t.expr(pwm).regs(i))
            :eq(addressmap.PWM_BASE + i * regs.CH1_CSR_OFFSET)
    end
end

local function setup(t)
    gpio.set_function(pin, gpio.FUNC_PWM)
    t:cleanup(function()
        gpio.deinit(pin)
        pwm.set_enabled(slice, false)
    end)
end

function test_config(t)
    setup(t)
    local cfg = pwm.get_default_config()
        :set_phase_correct(true)
        :set_clkdiv_int(1)
        :set_clkdiv_mode(pwm.DIV_FREE_RUNNING)
        :set_output_polarity(true, true)
        :set_wrap(1)
    t:expect(t.expr(cfg):csr()):eq(0x0e)
    t:expect(t.expr(cfg):div()):eq(1 << 4)
    t:expect(t.expr(cfg):top()):eq(1)

    pwm.init(slice, cfg, true)
    pwm.set_gpio_level(pin, 0)
    t:expect(t.expr(gpio).get_pad(pin)):eq(true)
    pwm.set_gpio_level(pin, 2)
    t:expect(t.expr(gpio).get_pad(pin)):eq(false)
end

function test_free_running(t)
    setup(t)
    pwm.set_clkdiv_int_frac(slice, 10)
    pwm.set_output_polarity(slice, false, false)
    pwm.set_clkdiv_mode(slice, pwm.DIV_FREE_RUNNING)
    pwm.set_phase_correct(slice, false)
    local wrap = 0xffff
    pwm.set_wrap(slice, wrap)

    pwm.set_counter(slice, 0)
    t:expect(t.expr(pwm).get_counter(slice)):eq(0)
    local value = wrap // 2
    pwm.set_chan_level(slice, chan, value)
    t:expect(t.expr(gpio).get_pad(pin)):eq(true)
    pwm.set_mask_enabled(1 << slice, true)
    t:expect(t.expr(pwm).get_counter(slice)):gt(0)
    while pwm.get_counter(slice) < value do end
    t:expect(t.expr(gpio).get_pad(pin)):eq(false)
    while pwm.get_counter(slice) >= value do end
    t:expect(t.expr(gpio).get_pad(pin)):eq(true)
end

function test_irq_handler(t)
    local slice_mask, want_counts = 0, {}
    for slice = 0, 7 do
        slice_mask = slice_mask | (1 << slice)
        pwm.set_clkdiv_int_frac(slice, 250)
        pwm.set_clkdiv_mode(slice, pwm.DIV_FREE_RUNNING)
        pwm.set_counter(slice, 0)
        pwm.set_wrap(slice, 4 * (3 * 5 * 7 * 8) / (1 + slice) - 1)
        want_counts[slice] = 1 + slice
        pwm.clear_irq(slice)
        pwm.set_irq_enabled(slice, true)
        t:cleanup(function() pwm.set_irq_enabled(slice, false) end)
    end

    local counts, parent = {}, thread.running()
    pwm.set_irq_handler(function(mask)
        if counts[0] then return end
        for i = 0, 7 do
            if mask & (1 << i) ~= 0 then counts[i] = (counts[i] or 0) + 1 end
        end
        if mask & 0x01 ~= 0 then parent:resume() end
    end)
    t:cleanup(function() pwm.set_irq_handler(nil) end)

    pwm.set_mask_enabled(slice_mask)
    t:cleanup(function() pwm.set_mask_enabled(0) end)
    thread.suspend()
    t:expect(counts):label("counts"):eq(want_counts, util.table_eq)
end
