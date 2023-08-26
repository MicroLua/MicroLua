_ENV = mlua.Module(...)

local gpio = require 'hardware.gpio'
local uart = require 'hardware.uart'
local list = require 'mlua.list'

pins = {
    [0] = {{0, 1, 2, 3}, {12, 13, 14, 15}, {16, 17, 18, 19}, {28, 29}},
    [1] = {{4, 5, 6, 7}, {8, 9, 10, 11}, {20, 21, 22, 23}, {24, 25, 26, 27}},
}

-- Returns the pins enabled for the given UART, as (TX, RX, CTS, RTS).
function find_pins(uart)
    local res = list.pack(nil, nil, nil, nil)
    for _, ps in ipairs(pins[uart:get_index()]) do
        for i, p in ipairs(ps) do
            if not res[i] and gpio.get_function(p) == gpio.FUNC_UART then
                res[i] = p
            end
        end
    end
    return res:unpack()
end

-- Returns the first UART that isn't the default UART, and its index.
function non_default(t)
    for i, u in ipairs(uart) do
        if u ~= uart.default then return u, i end
    end
    t:fatal("No non-default UART found")
end

