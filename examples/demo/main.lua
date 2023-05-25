_ENV = require 'mlua.module'(...)

local debug = require 'debug'
local gpio = require 'hardware.gpio'
local irq = require 'hardware.irq'
local timer = require 'hardware.timer'
local math = require 'math'
local eio = require 'mlua.eio'
local thread = require 'mlua.thread'
local pico = require 'pico'
local multicore = require 'pico.multicore'
local platform = require 'pico.platform'
local time = require 'pico.time'
local string = require 'string'

local irq_num = irq.user_irq_claim_unused()

local function task(id)
    for i = 1, 3 do
        eio.printf("Task: %s, i: %s\n", id, i)
        -- debug.debug()
        irq.set_pending(irq_num)
        thread.yield()
    end
    eio.printf("Task: %s, done\n", id)
end

function main()
    -- Wait for the system timer to start (it takes a while).
    timer.busy_wait_us(1)
    local start = time.get_absolute_time()

    eio.printf("============================\n")
    eio.printf("Start time: %s\n", start)
    eio.printf("Chip version: %d\n", platform.rp2040_chip_version())
    eio.printf("ROM version: %d\n", platform.rp2040_rom_version())
    eio.printf("Core: %d\n", platform.get_core_num())
    eio.printf("SDK: %s\n", pico.SDK_VERSION_STRING)
    multicore.launch_core1('main1')
    local core1_running = true

    local next_time = time.get_absolute_time() + 1500000
    next_time = next_time - (next_time % 1000000)
    timer.set_callback(0, function(alarm)
        local now = time.get_absolute_time()
        next_time = next_time + 1000000
        timer.set_target(0, next_time)
        eio.printf("# Alarm at %s, next at %s\n", now, next_time)
        if core1_running and now - start > 5000000 then
            eio.printf("# Resetting core 1\n")
            multicore.reset_core1()
            core1_running = false
        end
    end)
    timer.set_target(0, next_time)

    irq.set_handler(irq_num, function() eio.printf("# IRQ!\n") end)
    irq.clear(irq_num)
    irq.set_enabled(irq_num, true)

    -- for i = 1, 4 do
    --     thread.start(function() task(i) end)
    -- end

    local led = 21
    gpio.init(led)
    gpio.set_dir(led, gpio.OUT)
    local button = 17
    gpio.init(button)
    gpio.set_irq_callback(function(num, events)
        eio.printf("GPIO %s, events: %x\n", num, events)
        gpio.put(led, gpio.get(num))
    end)
    gpio.set_irq_enabled(button, gpio.IRQ_EDGE_FALL | gpio.IRQ_EDGE_RISE, true)
    -- gpio.set_irq_enabled(button, gpio.IRQ_LEVEL_HIGH, true)
    irq.set_enabled(irq.IO_IRQ_BANK0, true)

    -- while true do
    --     local b = gpio.get(button)
    --     gpio.put(led, b)
    --     thread.yield()
    -- end
end
