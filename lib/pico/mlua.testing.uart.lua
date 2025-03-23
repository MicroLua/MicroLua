-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local gpio = require 'hardware.gpio'
local uart = require 'hardware.uart'

pins = {
    [0] = {tx = {0, 12, 16, 28}, rx = {1, 13, 17, 29},
           cts = {2, 14, 18}, rts = {3, 15, 19}},
    [1] = {tx = {4, 8, 20, 24}, rx = {5, 9, 21, 25},
           cts = {6, 10, 22, 26}, rts = {7, 11, 23, 27}},
}

local function find_enabled(pins)
    for _, pin in ipairs(pins) do
        if gpio.get_function(pin) == gpio.FUNC_UART then return pin end
    end
end

-- Returns the pins enabled for the given UART, as (TX, RX, CTS, RTS).
function find_pins(inst)
    local ps = pins[inst:get_index()]
    return find_enabled(ps.tx), find_enabled(ps.rx),
           find_enabled(ps.cts), find_enabled(ps.rts)
end

-- Returns the first UART that isn't the default UART.
function non_default(t)
    for i = 0, uart.NUM - 1 do
        local inst = uart[i]
        if inst ~= uart.default then return inst end
    end
    t:fatal("No non-default UART found")
end

