_ENV = require 'mlua.module'(...)

local debug = require 'debug'
local clocks = require 'hardware.clocks'
local gpio = require 'hardware.gpio'
local irq = require 'hardware.irq'
local timer = require 'hardware.timer'
local uart = require 'hardware.uart'
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
local table = require 'table'

local function demo_sysinfo()
    eio.printf("Chip: %d, ROM: %d, core: %d, SDK: %s, Lua: %s\n",
        platform.rp2040_chip_version(), platform.rp2040_rom_version(),
        platform.get_core_num(), pico.SDK_VERSION_STRING,
        _RELEASE:gsub('[^ ]+ (.*)', '%1'))
    local id = unique_id.get_unique_board_id()
    eio.printf("Board ID: %s (%s)\n",
        id:gsub('(.)', function(c) return string.format('%02x', c:byte(1)) end),
        unique_id.get_unique_board_id_string())
    eio.printf("Flash: %08x, binary: %08x - %08x\n", pico.FLASH_SIZE_BYTES,
               pico.flash_binary_start, pico.flash_binary_end)
end

local function demo_clocks()
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
end

local function demo_core1()
    multicore.launch_core1('main1')
    thread.start(function()
        thread.sleep_for(5000000)
        eio.printf("Resetting core 1\n")
        multicore.reset_core1()
    end):set_name("Core 1 resetter")
end

local function demo_hardware_timer()
    local next_time = thread.now() + 1500000
    next_time = next_time - (next_time % 1000000)
    timer.set_target(0, next_time)
    thread.start(function()
        timer.enable_alarm(0)
        while true do
            timer.wait_alarm(0)
            local now = thread.now()
            next_time = next_time + 1000000
            timer.set_target(0, next_time)
            eio.printf("Alarm at %s\n", now)
        end
    end):set_name("Timer 0 handler")
end

local function demo_user_irq()
    local irq_num = irq.user_irq_claim_unused()
    thread.start(function()
        irq.enable_user_irq(irq_num)
        while true do
            irq.wait_user_irq(irq_num)
            eio.printf("User IRQ %d\n", irq_num)
        end
    end):set_name(string.format("User IRQ %d handler", irq_num))
    thread.start(function()
        thread.sleep_for(500000)
        for i = 0, 2 do
            eio.printf("Marking user IRQ %d pending\n", irq_num)
            irq.set_pending(irq_num)
            thread.sleep_for(500000)
        end
    end):set_name("User IRQ generator")
end


local function task(start)
    local name = thread.running():name()
    thread.sleep_until(start)
    for i = 0, 2 do
        eio.printf("%s, i: %s, t: %s\n", name, i, thread.now())
        -- debug.debug()
        thread.sleep_until(start + (i + 1) * 1000000)
    end
    eio.printf("%s, done\n", name)
end

local function demo_threads()
    local base = thread.now() + 1500000
    base = base - (base % 1000000)
    for i = 0, 3 do
        thread.start(function() task(base + i * 250000) end)
            :set_name(string.format("Task %d", i))
    end
end

local function demo_gpio()
    local leds = {21, 20, 19, 18}
    for _, led in ipairs(leds) do
        gpio.init(led)
        gpio.set_dir(led, gpio.OUT)
    end
    local buttons = {17, 16, 15, 14}
    for _, button in ipairs(buttons) do
        gpio.init(button)
        gpio.set_dir(button, gpio.IN)
        gpio.set_irq_enabled(button, gpio.IRQ_EDGE_FALL | gpio.IRQ_EDGE_RISE,
                             true)
        -- gpio.set_irq_enabled(button, gpio.IRQ_LEVEL_HIGH, true)
    end
    thread.start(function()
        gpio.enable_irq()
        while true do
            local evs = {gpio.wait_events(table.unpack(buttons))}
            for i, ev in ipairs(evs) do
                gpio.put(leds[i], gpio.get(buttons[i]))
                if ev ~= 0 then
                    eio.printf("GPIO %s, events: %x\n", buttons[i], ev)
                end
            end
        end
    end):set_name("GPIO event handler")
end

local function demo_stdin()
    thread.start(function()
        stdio.enable_chars_available()
        local line = ''
        while true do
            stdio.wait_chars_available()
            line = line .. stdin:read(32)  -- UART Rx FIFO is 32 bytes
            while true do
                local i = line:find('\n', 1, true)
                if not i then break end
                eio.printf("IN: %s", line:sub(1, i))
                line = line:sub(i + 1)
            end
        end
    end):set_name("stdin handler")
end

local function demo_uart()
    for i = 0, platform.NUM_UARTS - 1 do
        local u = uart[i]
        eio.printf("UART%d: %s\n", u:get_index(),
                   u:is_enabled() and "enabled" or "disabled")
    end
    local u = uart[1]
    u:init(1200)
    u:enable_loopback(true)
    u:enable_irq()
    local data = "The quick brown fox jumps over the lazy dog"
    thread.start(function()
        eio.printf("UART: Tx: start\n")
        u:write_blocking(data)
        u:tx_wait_blocking()
        eio.printf("UART: Tx: done\n")
    end):set_name("UART Tx")
    thread.start(function()
        local d = u:read_blocking(#data)
        eio.printf("UART: Rx: %s\n", d)
    end):set_name("UART Rx")
end

function main()
    local start = thread.now()

    -- Set the system clock.
    stdlib.set_sys_clock_khz(250000, true)
    -- stdlib.set_sys_clock_48mhz()
    stdlib.setup_default_uart()

    eio.printf(string.rep("=", 79) .. "\n")
    eio.printf("Main thread [%s], startup time: %s us\n",
               thread.running():name(), start)

    demo_sysinfo()
    demo_clocks()
    demo_core1()
    demo_hardware_timer()
    demo_user_irq()
    demo_threads()
    demo_gpio()
    demo_stdin()
    demo_uart()
end
