-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local clocks = require 'hardware.clocks'
local regs = require 'hardware.regs.clocks'
local testing_clocks = require 'mlua.testing.clocks'
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

local function expect_clk_sys(t, want)
    local _helper = t.helper
    t:expect(clocks.get_hz(clocks.clk_sys)):label("clk_sys"):eq(want)
end

function test_sys_clock(t)
    testing_clocks.restore_sys_clock(t)

    clocks.set_sys_clock_48mhz()
    testing_clocks.fix_uart_baudrate()
    expect_clk_sys(t, 48000000)

    local vco, div1, div2 = clocks.check_sys_clock_khz(123456789)
    t:expect(not vco, "Sys clock check unexpectedly succeeded")
    local vco, div1, div2 = clocks.check_sys_clock_khz(125000)
    t:expect(vco, "Sys clock check failed")

    testing_clocks.wait_uart_idle()
    clocks.set_sys_clock_pll(vco, div1, div2)
    testing_clocks.fix_uart_baudrate()
    expect_clk_sys(t, 125000000)

    testing_clocks.wait_uart_idle()
    local ok = clocks.set_sys_clock_khz(123456789)
    testing_clocks.fix_uart_baudrate()
    t:expect(not ok, "Sys clock change unexpectedly succeeded")

    testing_clocks.wait_uart_idle()
    local ok = clocks.set_sys_clock_khz(133000)
    testing_clocks.fix_uart_baudrate()
    t:expect(ok, "Sys clock change failed")
    expect_clk_sys(t, 133000000)
end
