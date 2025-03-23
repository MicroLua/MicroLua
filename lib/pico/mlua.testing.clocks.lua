-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local clocks = require 'hardware.clocks'
local uart = require 'hardware.uart'
local stdio = require 'pico.stdio'

function fix_uart_baudrate()
    if uart.default then uart.default:set_baudrate(uart.DEFAULT_BAUD_RATE) end
end

function wait_uart_idle()
    if uart.default then uart.default:tx_wait_blocking() end
end

function restore_sys_clock(t)
    local sys_khz = clocks.get_hz(clocks.clk_sys) // 1000
    t:cleanup(function()
        stdio.flush()
        wait_uart_idle()
        clocks.set_sys_clock_khz(sys_khz, true)
        fix_uart_baudrate()
    end)
    stdio.flush()
    wait_uart_idle()
end
