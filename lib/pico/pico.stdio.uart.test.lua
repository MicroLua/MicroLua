-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local gpio = require 'hardware.gpio'
local uart = require 'hardware.uart'
local list = require 'mlua.list'
local testing_uart = require 'mlua.testing.uart'
local pico = require 'pico'
local stdio_uart = require 'pico.stdio.uart'

local function set_function(pin, func)
    -- gpio.set_function() clears all overrides, so they can't be used
    -- to avoid spurious output or input. Pulls are in a different
    -- register and aren't affected, so we use that instead.
    local up, down = gpio.is_pulled_up(pin), gpio.is_pulled_down(pin)
    gpio.pull_up(pin)
    local old = gpio.get_function(pin)
    gpio.set_function(pin, func)
    return function()
        gpio.set_function(pin, old)
        gpio.set_pulls(pin, up, down)
    end
end

function test_init(t)
    local tx, rx = testing_uart.find_pins(uart.default)
    t:expect(tx):label("tx"):eq(pico.DEFAULT_UART_TX_PIN)
    t:expect(rx):label("rx"):eq(pico.DEFAULT_UART_RX_PIN)

    for _, test in ipairs{
        {'init', pico.DEFAULT_UART_TX_PIN, pico.DEFAULT_UART_RX_PIN},
        {'init_stdout', pico.DEFAULT_UART_TX_PIN, nil},
        {'init_stdin', nil, pico.DEFAULT_UART_RX_PIN},
    } do
        local fn, want_tx, want_rx = list.unpack(test, 1, 3)
        uart.default:tx_wait_blocking()
        local got_tx, got_rx
        do  -- No output in this block
            local tx_func<close> = set_function(tx, gpio.FUNC_NULL)
            local rx_func<close> = set_function(rx, gpio.FUNC_NULL)
            stdio_uart[fn]()
            got_tx, got_rx = testing_uart.find_pins(uart.default)
        end
        t:expect(got_tx):label("%s: tx", fn):eq(want_tx)
        t:expect(got_rx):label("%s: rx", fn):eq(want_rx)
    end

    -- Test init_full().
    uart.default:tx_wait_blocking()
    local u = testing_uart.non_default(t)
    local pins = testing_uart.pins[u:get_index()]
    local tx, rx = pins.tx[1], pins.rx[1]
    local got_tx, got_rx
    do  -- No output in this block
        local done<close> = function()
            stdio_uart.init()
            gpio.set_function(tx, gpio.FUNC_NULL)
            gpio.set_function(rx, gpio.FUNC_NULL)
            u:deinit()
        end
        stdio_uart.init_full(u, 1000000, tx, rx)
        got_tx, got_rx = testing_uart.find_pins(u)
    end
    t:expect(got_tx):label("init_full: tx"):eq(tx)
    t:expect(got_rx):label("init_full: rx"):eq(rx)
end
