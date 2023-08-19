_ENV = mlua.Module(...)

local gpio = require 'hardware.gpio'
local irq = require 'hardware.irq'
local list = require 'mlua.list'
local thread = require 'mlua.thread'
local pico = require 'pico'
local string = require 'string'

local pin = pico.DEFAULT_LED_PIN
local all_events = gpio.IRQ_LEVEL_LOW | gpio.IRQ_LEVEL_HIGH
                   | gpio.IRQ_EDGE_FALL | gpio.IRQ_EDGE_RISE

local function config_pin(t)
    if not pin then t:skip("DEFAULT_LED_PIN undefined") end
    gpio.init(pin)
    t:cleanup(function()
        gpio.deinit(pin)
        gpio.set_pulls(pin, false, true)
        gpio.set_irqover(pin, gpio.OVERRIDE_NORMAL)
        gpio.set_outover(pin, gpio.OVERRIDE_NORMAL)
        gpio.set_inover(pin, gpio.OVERRIDE_NORMAL)
        gpio.set_oeover(pin, gpio.OVERRIDE_NORMAL)
        gpio.set_input_hysteresis_enabled(pin, true)
        gpio.set_slew_rate(pin, gpio.SLEW_RATE_SLOW)
        gpio.set_drive_strength(pin, gpio.DRIVE_STRENGTH_4MA)
        gpio.set_irq_enabled(pin, all_events, false)
        gpio.set_dir(pin, gpio.IN)
    end)
end

function test_function(t)
    config_pin(t)
    t:expect(t:expr(gpio).get_function(pin)):eq(gpio.FUNC_SIO)
    gpio.set_function(pin, gpio.FUNC_NULL)
    t:expect(t:expr(gpio).get_function(pin)):eq(gpio.FUNC_NULL)
end

function test_pulls(t)
    config_pin(t)
    t:expect(t:expr(gpio).is_pulled_up(pin)):eq(false)
    t:expect(t:expr(gpio).is_pulled_down(pin)):eq(true)
    gpio.set_pulls(pin, true, false)
    t:expect(t:expr(gpio).is_pulled_up(pin)):eq(true)
    t:expect(t:expr(gpio).is_pulled_down(pin)):eq(false)
    gpio.pull_down(pin)
    t:expect(t:expr(gpio).is_pulled_up(pin)):eq(false)
    t:expect(t:expr(gpio).is_pulled_down(pin)):eq(true)
    gpio.pull_up(pin)
    t:expect(t:expr(gpio).is_pulled_up(pin)):eq(true)
    t:expect(t:expr(gpio).is_pulled_down(pin)):eq(false)
    gpio.disable_pulls(pin)
    t:expect(t:expr(gpio).is_pulled_up(pin)):eq(false)
    t:expect(t:expr(gpio).is_pulled_down(pin)):eq(false)
end

function test_overrides(t)
    config_pin(t)
    gpio.put(pin, 1)
    gpio.set_dir(pin, gpio.OUT)

    -- IRQ override
    irq.clear(irq.IO_IRQ_BANK0)
    gpio.set_irq_enabled(pin, gpio.IRQ_LEVEL_LOW, true)
    t:expect(t.expr.irq.is_pending(irq.IO_IRQ_BANK0)):eq(false)
    gpio.set_irqover(pin, gpio.OVERRIDE_INVERT)
    t:expect(t.expr.irq.is_pending(irq.IO_IRQ_BANK0)):eq(true)
    gpio.set_irqover(pin, gpio.OVERRIDE_NORMAL)

    -- Output override
    t:expect(t:expr(gpio).get(pin)):eq(true)
    gpio.set_outover(pin, gpio.OVERRIDE_INVERT)
    t:expect(t:expr(gpio).get(pin)):eq(false)
    gpio.set_outover(pin, gpio.OVERRIDE_NORMAL)

    -- Input override
    t:expect(t:expr(gpio).get(pin)):eq(true)
    gpio.set_inover(pin, gpio.OVERRIDE_LOW)
    t:expect(t:expr(gpio).get(pin)):eq(false)
    gpio.set_inover(pin, gpio.OVERRIDE_NORMAL)

    -- Output enable override
    t:expect(t:expr(gpio).get(pin)):eq(true)
    gpio.set_oeover(pin, gpio.OVERRIDE_LOW)
    t:expect(t:expr(gpio).get(pin)):eq(false)
    gpio.set_oeover(pin, gpio.OVERRIDE_NORMAL)
end

function test_input_hysteresis(t)
    config_pin(t)
    t:expect(t:expr(gpio).is_input_hysteresis_enabled(pin)):eq(true)
    gpio.set_input_hysteresis_enabled(pin, false)
    t:expect(t:expr(gpio).is_input_hysteresis_enabled(pin)):eq(false)
end

function test_slew_rate(t)
    config_pin(t)
    t:expect(t:expr(gpio).get_slew_rate(pin)):eq(gpio.SLEW_RATE_SLOW)
    gpio.set_slew_rate(pin, gpio.SLEW_RATE_FAST)
    t:expect(t:expr(gpio).get_slew_rate(pin)):eq(gpio.SLEW_RATE_FAST)
end

function test_drive_strength(t)
    config_pin(t)
    t:expect(t:expr(gpio).get_drive_strength(pin)):eq(gpio.DRIVE_STRENGTH_4MA)
    gpio.set_drive_strength(pin, gpio.DRIVE_STRENGTH_8MA)
    t:expect(t:expr(gpio).get_drive_strength(pin)):eq(gpio.DRIVE_STRENGTH_8MA)
end

function test_dir(t)
    config_pin(t)
    t:expect(t:expr(gpio).is_dir_out(pin)):eq(false)
    t:expect(t:expr(gpio).get_dir(pin)):eq(gpio.IN)
    gpio.set_dir_out_masked(1 << pin)
    t:expect(t:expr(gpio).is_dir_out(pin)):eq(true)
    t:expect(t:expr(gpio).get_dir(pin)):eq(gpio.OUT)
    gpio.set_dir_in_masked(1 << pin)
    t:expect(t:expr(gpio).is_dir_out(pin)):eq(false)
    t:expect(t:expr(gpio).get_dir(pin)):eq(gpio.IN)
    gpio.set_dir_masked(1 << pin, 1 << pin)
    t:expect(t:expr(gpio).is_dir_out(pin)):eq(true)
    t:expect(t:expr(gpio).get_dir(pin)):eq(gpio.OUT)
    gpio.set_dir_masked(1 << pin, 0)
    t:expect(t:expr(gpio).is_dir_out(pin)):eq(false)
    t:expect(t:expr(gpio).get_dir(pin)):eq(gpio.IN)
    local v = 0
    for i = 0, 31 do v = v | (gpio.is_dir_out(pin) and (1 << i) or 0) end
    gpio.set_dir_all_bits(v | (1 << pin))
    t:expect(t:expr(gpio).is_dir_out(pin)):eq(true)
    t:expect(t:expr(gpio).get_dir(pin)):eq(gpio.OUT)
    gpio.set_dir(pin, gpio.IN)
    t:expect(t:expr(gpio).is_dir_out(pin)):eq(false)
    t:expect(t:expr(gpio).get_dir(pin)):eq(gpio.IN)
end

function test_init(t)
    if not pin then t:skip("DEFAULT_LED_PIN undefined") end
    t:cleanup(function() gpio.deinit(pin) end)
    t:expect(t:expr(gpio).get_function(pin)):eq(gpio.FUNC_NULL)
    gpio.init(pin)
    t:expect(t:expr(gpio).get_function(pin)):eq(gpio.FUNC_SIO)
    gpio.deinit(pin)
    t:expect(t:expr(gpio).get_function(pin)):eq(gpio.FUNC_NULL)
    gpio.init_mask(1 << pin)
    t:expect(t:expr(gpio).get_function(pin)):eq(gpio.FUNC_SIO)
end

function test_driving(t)
    config_pin(t)
    gpio.put(pin, 0)
    gpio.set_dir(pin, gpio.OUT)

    gpio.put(pin, 1)
    t:expect(t:expr(gpio).get_out_level(pin)):eq(true)
    t:expect(t:expr(gpio).get(pin)):eq(true)
    t:expect(gpio.get_all() & (1 << pin)):label("gpio.get_all()")
        :eq((1 << pin))
    t:expect(t:expr(gpio).get_pad(pin)):eq(true)

    gpio.set_input_enabled(pin, false)
    t:expect(t:expr(gpio).get(pin)):eq(false)
    t:expect(gpio.get_all() & (1 << pin)):label("gpio.get_all()"):eq(0)
    gpio.set_input_enabled(pin, true)

    gpio.put(pin, 0)
    t:expect(t:expr(gpio).get_out_level(pin)):eq(false)
    t:expect(t:expr(gpio).get(pin)):eq(false)
    t:expect(gpio.get_all() & (1 << pin)):label("gpio.get_all()"):eq(0)
    t:expect(t:expr(gpio).get_pad(pin)):eq(false)

    gpio.set_mask(1 << pin)
    t:expect(t:expr(gpio).get_out_level(pin)):eq(true)
    gpio.clr_mask(1 << pin)
    t:expect(t:expr(gpio).get_out_level(pin)):eq(false)
    gpio.xor_mask(1 << pin)
    t:expect(t:expr(gpio).get_out_level(pin)):eq(true)
    gpio.put_masked(1 << pin, 0)
    t:expect(t:expr(gpio).get_out_level(pin)):eq(false)
    local v = 0
    for i = 0, 31 do v = v | (gpio.get_out_level(pin) and (1 << i) or 0) end
    gpio.put_all(v | (1 << pin))
    t:expect(t:expr(gpio).get_out_level(pin)):eq(true)
end

function test_irq_events(t)
    config_pin(t)
    gpio.put(pin, 0)
    gpio.set_dir(pin, gpio.OUT)
    gpio.set_irq_enabled(pin,
        gpio.IRQ_LEVEL_HIGH | gpio.IRQ_EDGE_FALL | gpio.IRQ_EDGE_RISE, true)

    t:expect(t:expr(gpio).get_irq_event_mask(pin)):eq(0)
    gpio.put(pin, 1)
    t:expect(t:expr(gpio).get_irq_event_mask(pin))
        :eq(gpio.IRQ_LEVEL_HIGH | gpio.IRQ_EDGE_RISE)
    gpio.acknowledge_irq(pin, all_events)
    t:expect(t:expr(gpio).get_irq_event_mask(pin)):eq(gpio.IRQ_LEVEL_HIGH)
    gpio.put(pin, 0)
    t:expect(t:expr(gpio).get_irq_event_mask(pin)):eq(gpio.IRQ_EDGE_FALL)
    gpio.acknowledge_irq(pin, all_events)
    t:expect(t:expr(gpio).get_irq_event_mask(pin)):eq(0)
end

function test_irq_handling(t)
    config_pin(t)
    gpio.put(pin, 0)
    gpio.set_dir(pin, gpio.OUT)
    gpio.set_irq_enabled(pin,
        gpio.IRQ_LEVEL_HIGH | gpio.IRQ_EDGE_FALL | gpio.IRQ_EDGE_RISE, true)

    -- Start an event watcher.
    local events = {'LEVEL_LOW', 'LEVEL_HIGH', 'EDGE_FALL', 'EDGE_RISE'}
    local log, stop = '', false
    local watcher<close> = thread.start(function()
        gpio.enable_irq()
        local done<close> = function() gpio.enable_irq(false) end
        while not stop do
            local es = gpio.wait_events(pin)
            local names = list()
            for _, e in ipairs(events) do
                if es & gpio[('IRQ_%s'):format(e)] ~= 0 then names:append(e) end
            end
            log = log .. ('(%s) '):format(names:concat(' '))
            thread.yield()
        end
    end)

    -- Trigger events and let the watcher record.
    thread.yield()  -- Let the watcher initialize and block
    gpio.put(pin, 1)
    thread.yield()
    thread.yield()
    gpio.put(pin, 0)
    thread.yield()
    t:expect(log):label("log")
        :eq('(LEVEL_HIGH EDGE_RISE) (LEVEL_HIGH) (EDGE_FALL) ')
    stop = true
    gpio.put(pin, 1)
end
