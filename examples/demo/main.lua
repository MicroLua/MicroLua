_ENV = require 'mlua.module'(...)

local debug = require 'debug'
local clocks = require 'hardware.clocks'
local gpio = require 'hardware.gpio'
local irq = require 'hardware.irq'
local timer = require 'hardware.timer'
local math = require 'math'
local eio = require 'mlua.eio'
local thread = require 'mlua.thread'
local pico = require 'pico'
local multicore = require 'pico.multicore'
local platform = require 'pico.platform'
local stdio = require 'pico.stdio'
local stdlib = require 'pico.stdlib'
local time = require 'pico.time'
local unique_id = require 'pico.unique_id'
local string = require 'string'

local irq_num = irq.user_irq_claim_unused()

local function task(id, start)
    thread.yield(start)
    for i = 0, 2 do
        eio.printf("Task: %s (%s), i: %s, t: %s\n", id, thread.running(), i,
                   thread.now())
        -- debug.debug()
        irq.set_pending(irq_num)
        thread.yield(start + (i + 1) * 1000000)
    end
    eio.printf("Task: %s, done\n", id)
end

function main()
    -- Switch the system clock.
    stdlib.set_sys_clock_khz(200000, true)
    -- stdlib.set_sys_clock_48mhz()
    stdlib.setup_default_uart()

    -- Wait for the system timer to start (it takes a while).
    timer.busy_wait_us(1)
    local start = time.get_absolute_time()

    eio.printf("============================\n")
    eio.printf("Start time: %s\n", start)
    eio.printf("Chip version: %d\n", platform.rp2040_chip_version())
    eio.printf("ROM version: %d\n", platform.rp2040_rom_version())
    local id = unique_id.get_unique_board_id()
    eio.printf("Board ID: %s (%s)\n",
        id:gsub('(.)', function(c) return string.format('%02x', c:byte(1)) end),
        unique_id.get_unique_board_id_string())
    eio.printf("Core: %d\n", platform.get_core_num())
    eio.printf("SDK: %s\n", pico.SDK_VERSION_STRING)

    eio.printf("pll_sys : %6d kHz\n", clocks.frequency_count_khz(
        clocks.FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY))
    eio.printf("pll_usb : %6d kHz\n", clocks.frequency_count_khz(
        clocks.FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY))
    eio.printf("rosc    : %6d kHz\n", clocks.frequency_count_khz(
        clocks.FC0_SRC_VALUE_ROSC_CLKSRC))
    eio.printf("clk_sys : %6d kHz\n", clocks.frequency_count_khz(
        clocks.FC0_SRC_VALUE_CLK_SYS))
    eio.printf("clk_peri: %6d kHz\n", clocks.frequency_count_khz(
        clocks.FC0_SRC_VALUE_CLK_PERI))
    eio.printf("clk_usb : %6d kHz\n", clocks.frequency_count_khz(
        clocks.FC0_SRC_VALUE_CLK_USB))
    eio.printf("clk_adc : %6d kHz\n", clocks.frequency_count_khz(
        clocks.FC0_SRC_VALUE_CLK_ADC))
    eio.printf("clk_rtc : %6d kHz\n", clocks.frequency_count_khz(
        clocks.FC0_SRC_VALUE_CLK_RTC))

    -- multicore.launch_core1('main1')
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
    -- timer.set_target(0, next_time)

    irq.set_handler(irq_num, function() eio.printf("# IRQ!\n") end)
    irq.clear(irq_num)
    -- irq.set_enabled(irq_num, true)

    local base = thread.now() + 1500000
    base = base - (base % 1000000)
    for i = 0, 3 do
        -- thread.start(function() task(i, base + i * 250000) end)
    end

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
end
