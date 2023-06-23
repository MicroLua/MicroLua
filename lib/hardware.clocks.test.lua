_ENV = require 'mlua.module'(...)

local clocks = require 'hardware.clocks'
local table = require 'table'

function test_frequency(t)
    for _, test in ipairs{
        {'FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY', 'clk_sys', 0.0001},
        {'FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY', 'clk_usb', 0.0001},
        {'FC0_SRC_VALUE_ROSC_CLKSRC', 6500.0, 0.85},
        {'FC0_SRC_VALUE_CLK_SYS', 'clk_sys', 0.0001},
        {'FC0_SRC_VALUE_CLK_PERI', 'clk_peri', 0.0001},
        {'FC0_SRC_VALUE_CLK_USB', 'clk_usb', 0.0001},
        {'FC0_SRC_VALUE_CLK_ADC', 'clk_adc', 0.0001},
        {'FC0_SRC_VALUE_CLK_RTC', 'clk_rtc', 0.03},
    } do
        local clk, want, tol = table.unpack(test)
        if type(want) == 'string' then
            want = clocks.get_hz(clocks[want]) / 1000
        end
        local got = clocks.frequency_count_khz(clocks[clk])
        t:expect((1.0 - tol) * want <= got and got <= (1.0 + tol) * want,
                 "freq(%s) = %s, want %.0f", clk, got, want)
    end
end
