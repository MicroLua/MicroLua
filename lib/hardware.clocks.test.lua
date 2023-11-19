-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = mlua.module(...)

local clocks = require 'hardware.clocks'
local regs = require 'hardware.regs.clocks'
local table = require 'table'

function test_frequency_count_khz(t)
    for _, test in ipairs{
        {'PLL_SYS_CLKSRC_PRIMARY', 'clk_sys', 0.0001},
        {'PLL_USB_CLKSRC_PRIMARY', 'clk_usb', 0.0001},
        {'ROSC_CLKSRC', 6500.0, 0.85},
        {'CLK_SYS', 'clk_sys', 0.0001},
        {'CLK_PERI', 'clk_peri', 0.0001},
        {'CLK_USB', 'clk_usb', 0.0001},
        {'CLK_ADC', 'clk_adc', 0.0001},
        {'CLK_RTC', 'clk_rtc', 0.03},
    } do
        local clk, want, tol = table.unpack(test)
        if type(want) == 'string' then
            want = clocks.get_hz(clocks[want]) / 1000
        end
        local clkid = regs['FC0_SRC_VALUE_' .. clk]
        t:expect(clocks.frequency_count_khz(clkid))
            :label("frequency_count_khz(%s)", clk):close_to_rel(want, tol)
    end
end
