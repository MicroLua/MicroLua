_ENV = require 'mlua.module'(...)

local adc = require 'hardware.adc'
local gpio = require 'hardware.gpio'
local thread = require 'mlua.thread'
local pico = require 'pico'
local string = require 'string'

local TEMPERATURE_UNIT = "C"
local conv = 3.3 / (1 << 12)

local function onboard_temperature(value, unit)
    local temp = 27.0 - (value * conv - 0.706) / 0.001721
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
    adc.fifo_setup(true, false, 1, false, false)
    adc.set_clkdiv(48000 - 1)
    adc.fifo_drain()
    adc.fifo_enable_irq()
    adc.run(true)

    -- Read from the ADC FIFO.
    local cnt = 0
    while true do
        local value = adc.fifo_get_blocking()
        cnt = cnt + 1
        if cnt == 1000 then
            cnt = 0
            local temp = onboard_temperature(value, TEMPERATURE_UNIT)
            print(string.format("Onboard temperature = %.2f %s", temp,
                                TEMPERATURE_UNIT))
            if LED_PIN then gpio.xor_mask(1 << LED_PIN) end
        end
    end
end
