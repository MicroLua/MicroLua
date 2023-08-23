_ENV = mlua.Module(...)

local gpio = require 'hardware.gpio'
local uart = require 'hardware.uart'
local stdio = require 'pico.stdio'
local stdio_uart = require 'pico.stdio.uart'

local uart_tx_pins = {[0] = {0, 12, 16, 28}, [1] = {4, 8, 20, 24}}

local function find_uart_tx_pin(uart)
    for _, pin in ipairs(uart_tx_pins[uart:get_index()]) do
        if gpio.get_function(pin) == gpio.FUNC_UART then return pin end
    end
end

function enable_loopback(t, crlf)
    t:assert(stdio_uart.driver, "UART stdio driver unavailable")
    stdio.flush()

    -- Restrict stdio to UART only.
    stdio.set_driver_enabled(stdio_uart.driver, true)
    stdio.filter_driver(stdio_uart.driver)
    stdio.set_translate_crlf(stdio_uart.driver, crlf)
    if yield_enabled() then stdio.enable_chars_available() end

    -- Enable UART loopback, and prevent output data from actually appearing at
    -- the Tx pin.
    local u = uart.default
    local tx = find_uart_tx_pin(u)
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
        if yield_enabled() then stdio.enable_chars_available(false) end
        stdio.set_translate_crlf(stdio_uart.driver, stdio.DEFAULT_CRLF)
        stdio.filter_driver(nil)
    end
end
