_ENV = mlua.Module(...)

local gpio = require 'hardware.gpio'
local uart = require 'hardware.uart'
local list = require 'mlua.list'
local thread = require 'mlua.thread'
local stdio = require 'pico.stdio'
local stdio_uart = require 'pico.stdio.uart'
local string = require 'string'

-- We don't test the pico.stdio.semihosting module, because it requires a
-- debugger to be attached. Attempts to catch bkpt instructions through the
-- hardfault handler have failed; the handler doesn't seem to be called, and
-- the core just locks up.

-- We don't test the pico.stdio.usb module, because it sometimes causes the
-- xhci_hcd driver to lock up and terminate, thereby disconnecting all USB
-- devices and requiring a reboot.

local uart_tx_pins = {[0] = {0, 12, 16, 28}, [1] = {4, 8, 20, 24}}

local function find_uart_tx_pin(uart)
    for _, pin in ipairs(uart_tx_pins[uart:get_index()]) do
        if gpio.get_function(pin) == gpio.FUNC_UART then return pin end
    end
end

local function enable_stdio_loopback(t, crlf)
    t:assert(stdio_uart.driver ~= nil, "UART stdio driver unavailable")
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
        gpio.set_outover(
            tx, gpio.get_pad(tx) and gpio.OVERRIDE_HIGH or gpio.OVERRIDE_LOW)
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

function test_putchar_getchar_Y(t)
    for _, test in ipairs{
        {false, {65, 10, 66, 13, 10, 67},
                {65, 10, 66, 13, 10, 67, 65, 10, 66, 13, 10, 67, -1}},
        {true, {65, 10, 66, 13, 10, 67},
               {65, 10, 66, 13, 10, 67, 65, 13, 10, 66, 13, 10, 67, -1}},
    } do
        local crlf, chars, want = list.unpack(test)
        local got = list()
        t:expect(pcall(function()
            local done<close> = enable_stdio_loopback(t, crlf)
            for _, c in ipairs(chars) do stdio.putchar_raw(c) end
            for _, c in ipairs(chars) do stdio.putchar(c) end
            for i = 1, 4 do got:append(stdio.getchar()) end
            for i = 5, #want do got:append(stdio.getchar_timeout_us(1000)) end
        end))
        t:expect(got):label("crlf: %s, got", crlf):eq(want, list.eq)
    end
end

function test_puts(t)
    for _, test in ipairs{
        {false, 'abcdef', 'abcdef\nabcdef\n'},
        {true, 'abcdef', 'abcdef\nabcdef\r\n'},
    } do
        local crlf, s, want = list.unpack(test)
        local got = ''
        t:expect(pcall(function()
            local done<close> = enable_stdio_loopback(t, crlf)
            stdio.puts_raw(s)
            stdio.puts(s)
            for i = 1, #want do got = got .. string.char(stdio.getchar()) end
        end))
        t:expect(got):label("crlf: %s, got", crlf):eq(want)
    end
end

function test_chars_available(t)
    local got, want = '', 'abcdefghijklmnopqrstuvwxyz'
    t:expect(pcall(function()
        local done<close> = enable_stdio_loopback(t, false)
        local reader<close> = thread.start(function()
            while true do
                stdio.wait_chars_available()
                local c = stdio.getchar()
                if c == 10 then return end
                got = got .. string.char(c)
            end
        end)
        stdio.puts_raw(want)
    end))
    t:expect(got):label("got"):eq(want)
end
