-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local gpio = require 'hardware.gpio'
local uart = require 'hardware.uart'
local testing_uart = require 'mlua.testing.uart'
local stdio = require 'pico.stdio'
local stdlib = require 'pico.stdlib'

function set_up(t)
    -- Pull up the default UART Tx & Rx pins, to avoid spurious input if they
    -- aren't connected.
    local u = uart.default
    if u then
        local tx, rx = testing_uart.find_pins(u)
        if rx then
            gpio.pull_up(tx)
            gpio.pull_up(rx)
            while stdio.getchar_timeout_us(1000) >= 0 do end
        end
        u:tx_wait_blocking()
    end

    -- Set a faster clock speed.
    stdlib.set_sys_clock_khz(250000, true)
    if u then u:set_baudrate(uart.DEFAULT_BAUD_RATE) end
end
