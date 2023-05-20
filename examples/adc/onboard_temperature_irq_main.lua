_ENV = require 'module'(...)

local adc = require 'hardware.adc'
local gpio = require 'hardware.gpio'
local irq = require 'hardware.irq'
local pico = require 'pico'
local string = require 'string'
local thread = require 'thread'

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

    -- Set up the ADC IRQ handler.
    local cnt = 0
    adc.irq_set_handler(function()
        while not adc.fifo_is_empty() do
            local value = adc.fifo_get()
            cnt = cnt + 1
            if cnt == 1000 then
                cnt = 0
                local temp = onboard_temperature(value, TEMPERATURE_UNIT)
                print(string.format("Onboard temperature = %.02f %s", temp,
                                    TEMPERATURE_UNIT))
                if LED_PIN then gpio.xor_mask(1 << LED_PIN) end
            end
        end
    end)

    -- Start the ADC in free-running mode.
    adc.fifo_drain()
    adc.irq_set_enabled(true)
    irq.set_enabled(irq.ADC_IRQ_FIFO, true)
    adc.run(true)
end
