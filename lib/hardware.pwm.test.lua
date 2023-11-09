-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = mlua.Module(...)

local gpio = require 'hardware.gpio'
local pwm = require 'hardware.pwm'
local addressmap = require 'hardware.regs.addressmap'
local regs = require 'hardware.regs.pwm'
local pico = require 'pico'

function test_introspect(t)
    t:expect(t:expr(pwm).gpio_to_slice_num(13)):eq(6)
    t:expect(t:expr(pwm).gpio_to_channel(13)):eq(pwm.CHAN_B)
    t:expect(t:expr(pwm).reg_base()):eq(addressmap.PWM_BASE)
    t:expect(t:expr(pwm).reg_base(3))
        :eq(addressmap.PWM_BASE + regs.CH3_CSR_OFFSET)
end

local function setup(t)
    local pin = pico.DEFAULT_LED_PIN
    gpio.set_function(pin, gpio.FUNC_PWM)
    local slice = pwm.gpio_to_slice_num(pin)
    t:cleanup(function()
        gpio.deinit(pin)
        pwm.set_enabled(slice, false)
    end)
    return pin, slice, pwm.gpio_to_channel(pin)
end

function test_config(t)
    pin, slice = setup(t)
    local cfg = pwm.get_default_config()
    cfg:set_phase_correct(false)
    cfg:set_clkdiv_int(1)
    cfg:set_clkdiv_mode(pwm.DIV_FREE_RUNNING)
    cfg:set_output_polarity(false, false)
    cfg:set_wrap(1)
    pwm.init(slice, cfg, true)

    pwm.set_gpio_level(pin, 0)
    t:expect(t:expr(gpio).get_pad(pin)):eq(false)
    pwm.set_gpio_level(pin, 2)
    t:expect(t:expr(gpio).get_pad(pin)):eq(true)
end

function test_free_running(t)
    pin, slice, chan = setup(t)
    pwm.set_clkdiv_int_frac(slice, 32)
    pwm.set_output_polarity(slice, false, false)
    pwm.set_clkdiv_mode(slice, pwm.DIV_FREE_RUNNING)
    pwm.set_phase_correct(slice, false)
    local wrap = 0xffff
    pwm.set_wrap(slice, wrap)

    pwm.set_counter(slice, 0)
    t:expect(t:expr(pwm).get_counter(slice)):eq(0)
    local value = wrap // 2
    pwm.set_chan_level(slice, chan, value)
    pwm.set_mask_enabled(1 << slice, true)
    t:expect(t:expr(pwm).get_counter(slice)):gt(0)
    t:expect(t:expr(gpio).get_pad(pin)):eq(true)
    while pwm.get_counter(slice) < value do end
    t:expect(t:expr(gpio).get_pad(pin)):eq(false)
    while pwm.get_counter(slice) >= value do end
    t:expect(t:expr(gpio).get_pad(pin)):eq(true)
end
