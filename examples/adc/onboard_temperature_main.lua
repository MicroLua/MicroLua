_ENV = require 'mlua.module'(...)

local adc = require 'hardware.adc'
local gpio = require 'hardware.gpio'
local pico = require 'pico'
local time = require 'pico.time'
local string = require 'string'

local TEMPERATURE_UNIT = "C"
local conv = 3.3 / (1 << 12)

local function read_onboard_temperature(unit)
    local value = adc.read() * conv
    local temp = 27.0 - (value - 0.706) / 0.001721
    if unit == "F" then temp = temp * 9 / 5 + 32 end
    return temp
end

function main()
    -- Set up the LED pin.
    local LED_PIN = pico.DEFAULT_LED_PIN
    if LED_PIN then
        gpio.init(LED_PIN)
        gpio.set_dir(LED_PIN, gpio.OUT)
    end

    -- Configure the ADC.
    adc.init()
    adc.set_temp_sensor_enabled(true)
    adc.select_input(4)

    -- Perform conversions.
    while true do
        local temp = read_onboard_temperature(TEMPERATURE_UNIT)
        print(string.format("Onboard temperature = %.02f %s", temp,
                            TEMPERATURE_UNIT))
        if LED_PIN then
            gpio.put(LED_PIN, 1)
            time.sleep_ms(10)
            gpio.put(LED_PIN, 0)
        end
        time.sleep_ms(990)
    end
end
