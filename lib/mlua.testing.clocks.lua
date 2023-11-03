-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = mlua.Module(...)

local clocks = require 'hardware.clocks'
local uart = require 'hardware.uart'
local stdio = require 'pico.stdio'
local stdlib = require 'pico.stdlib'

function wait_uart_idle()
    if uart.default then uart.default:tx_wait_blocking() end
end

function restore_sys_clock(t)
    local sys_khz = clocks.get_hz(clocks.clk_sys) // 1000
    t:cleanup(function()
        stdio.flush()
        wait_uart_idle()
        stdlib.set_sys_clock_khz(sys_khz, true)
        stdlib.setup_default_uart()
    end)
    stdio.flush()
    wait_uart_idle()
end
