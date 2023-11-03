-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = mlua.Module(...)

local clocks = require 'hardware.clocks'
local testing_clocks = require 'mlua.testing.clocks'
local stdlib = require 'pico.stdlib'

local function expect_clk_sys(t, want)
    local _helper = t.helper
    t:expect(clocks.get_hz(clocks.clk_sys)):label("clk_sys"):eq(want)
end

function test_sys_clock(t)
    testing_clocks.restore_sys_clock(t)

    stdlib.set_sys_clock_48mhz()
    stdlib.setup_default_uart()
    expect_clk_sys(t, 48000000)

    local vco, div1, div2 = stdlib.check_sys_clock_khz(123456789)
    t:expect(not vco, "Sys clock check unexpectedly succeeded")
    local vco, div1, div2 = stdlib.check_sys_clock_khz(125000)
    t:expect(vco, "Sys clock check failed")

    testing_clocks.wait_uart_idle()
    stdlib.set_sys_clock_pll(vco, div1, div2)
    stdlib.setup_default_uart()
    expect_clk_sys(t, 125000000)

    testing_clocks.wait_uart_idle()
    local ok = stdlib.set_sys_clock_khz(123456789)
    stdlib.setup_default_uart()
    t:expect(not ok, "Sys clock change unexpectedly succeeded")

    testing_clocks.wait_uart_idle()
    local ok = stdlib.set_sys_clock_khz(133000)
    stdlib.setup_default_uart()
    t:expect(ok, "Sys clock change failed")
    expect_clk_sys(t, 133000000)
end
