_ENV = mlua.Module(...)

local clocks = require 'hardware.clocks'
local uart = require 'hardware.uart'
local stdlib = require 'pico.stdlib'

function wait_uart_idle()
    if uart.default then uart.default:tx_wait_blocking() end
end

function expect_clk_sys(t, want)
    local _helper = t.helper
    local got = clocks.get_hz(clocks.clk_sys)
    t:expect(got == want, "Unexpected clk_sys freq: got %s, want %s", got, want)
end

function test_sys_clock(t)
    local save = clocks.get_hz(clocks.clk_sys)
    t:cleanup(function()
        wait_uart_idle()
        stdlib.set_sys_clock_khz(save)
        stdlib.setup_default_uart()
    end)

    wait_uart_idle()
    stdlib.set_sys_clock_48mhz()
    stdlib.setup_default_uart()
    expect_clk_sys(t, 48000000)

    local vco, div1, div2 = stdlib.check_sys_clock_khz(123456789)
    t:expect(not vco, "Sys clock check unexpectedly succeeded")
    local vco, div1, div2 = stdlib.check_sys_clock_khz(125000)
    t:expect(vco, "Sys clock check failed")

    wait_uart_idle()
    stdlib.set_sys_clock_pll(vco, div1, div2)
    stdlib.setup_default_uart()
    expect_clk_sys(t, 125000000)

    wait_uart_idle()
    local ok = stdlib.set_sys_clock_khz(123456789)
    stdlib.setup_default_uart()
    t:expect(not ok, "Sys clock change unexpectedly succeeded")

    wait_uart_idle()
    local ok = stdlib.set_sys_clock_khz(133000)
    stdlib.setup_default_uart()
    t:expect(ok, "Sys clock change failed")
    expect_clk_sys(t, 133000000)
end
