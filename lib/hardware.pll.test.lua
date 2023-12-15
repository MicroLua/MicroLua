-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local clocks = require 'hardware.clocks'
local regs = require 'hardware.regs.clocks'
local pll = require 'hardware.pll'
local testing_clocks = require 'mlua.testing.clocks'
local stdlib = require 'pico.stdlib'

function test_init(t)
    testing_clocks.restore_sys_clock(t)

    -- Switch to pll_usb. This turns off pll_sys.
    stdlib.set_sys_clock_48mhz()
    stdlib.setup_default_uart()
    local clk = regs.FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY
    t:expect(clocks.frequency_count_khz(clk)):label("pll_sys"):eq(0)

    -- Configure and measure pll_sys.
    t:cleanup(function() pll.deinit(pll.sys) end)
    for _, test in ipairs{100000, 125000, 133000} do
        local vco, div1, div2 = stdlib.check_sys_clock_khz(test)
        t:assert(vco, "check_sys_clock_khz(%s) failed", test)
        pll.init(pll.sys, 1, vco, div1, div2)
        t:expect(clocks.frequency_count_khz(clk)):label("pll_sys")
            :close_to_rel(test, 0.0001)
    end
end
