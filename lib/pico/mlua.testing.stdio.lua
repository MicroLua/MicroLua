-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local gpio = require 'hardware.gpio'
local uart = require 'hardware.uart'
local testing_uart = require 'mlua.testing.uart'
local thread = try(require, 'mlua.thread')
local stdio = require 'pico.stdio'
local stdio_uart = require 'pico.stdio.uart'

function enable_loopback(t, crlf)
    t:assert(stdio_uart.driver, "UART stdio driver unavailable")
    stdio.flush()

    -- Restrict stdio to UART only.
    stdio.set_driver_enabled(stdio_uart.driver, true)
    stdio.filter_driver(stdio_uart.driver)
    stdio.set_translate_crlf(stdio_uart.driver, crlf)
    if thread and not thread.blocking() then stdio.enable_chars_available() end

    -- Enable UART loopback, and prevent output data from actually appearing at
    -- the Tx pin.
    local u = uart.default
    local tx = testing_uart.find_pins(u)
    u:tx_wait_blocking()
    if tx then
        gpio.set_outover(tx, gpio.get_pad(tx) and gpio.OVERRIDE_HIGH
                                               or gpio.OVERRIDE_LOW)
    end
    u:enable_loopback(true)

    return function()
        stdio.flush()
        u:tx_wait_blocking()
        while stdio.getchar_timeout_us(1000) >= 0 do end
        u:enable_loopback(false)
        if tx then gpio.set_outover(tx, gpio.OVERRIDE_NORMAL) end
        if thread and not thread.blocking() then
            stdio.enable_chars_available(false)
        end
        stdio.set_translate_crlf(stdio_uart.driver, stdio.DEFAULT_CRLF)
        stdio.filter_driver(nil)
    end
end
