_ENV = mlua.Module(...)

local gpio = require 'hardware.gpio'
local pico = require 'pico'
local time = require 'pico.time'

function main()
    local LED_PIN = pico.DEFAULT_LED_PIN
    gpio.init(LED_PIN)
    gpio.set_dir(LED_PIN, gpio.OUT)
    while true do
        gpio.put(LED_PIN, 1)
        time.sleep_ms(250)
        gpio.put(LED_PIN, 0)
        time.sleep_ms(250)
    end
end
