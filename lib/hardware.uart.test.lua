_ENV = mlua.Module(...)

local uart = require 'hardware.uart'
local math = require 'math'
local thread = require 'mlua.thread'

function test_strict(t)
    local ok = pcall(function() return uart.UNKNOWN end)
    t:expect(not ok, "module is non-strict")
    local ok = pcall(function() return uart[0].UNKNOWN end)
    t:expect(not ok, "Uart instance is non-strict")
end

local function non_default_uart(t)
    for i, u in ipairs(uart) do
        if u ~= uart.default then return u, i end
    end
    t:fatal("No non-default UART found")
end

local function setup(t)
    local u, idx = non_default_uart(t)
    u:init(115200)
    t:cleanup(function() u:deinit() end)
    u:enable_loopback(true)
    u:enable_irq()
    u:tx_wait_blocking()
    while u:is_readable() do u:read_blocking(1) end
    return u, idx
end

function test_configuration(t)
    local u, idx = setup(t)
    t:expect(t:expr(u):get_index()):eq(idx)
    u:enable_irq(false)
    local baud = 115200
    t:expect(t:expr(u):set_baudrate(baud)):close_to_rel(baud, 0.05)
    u:set_format(7, 1, uart.PARITY_ODD)
    u:set_hw_flow(false, false)
    u:set_fifo_enabled(true)
    u:enable_irq()
    local data = 'abc\x81\x92\xa3'
    u:write_blocking(data)
    t:expect(t:expr(u):read_blocking(#data)):eq('abc\x01\x12\x23')
end

function test_blocking_write_read_Y(t)
    local u = setup(t)
    local data = "0123456789abcdefghijklmnopqrstuv"  -- Fits the FIFO
    t:expect(t:expr(u):is_enabled()):eq(true)
    t:expect(t:expr(u):is_writable()):eq(true)
    t:expect(t:expr(u):is_tx_busy()):eq(false)
    t:expect(t:expr(u):is_readable()):eq(false)
    u:write_blocking(data)
    t:expect(t:expr(u):is_tx_busy()):eq(true)
    u:tx_wait_blocking()
    t:expect(t:expr(u):is_tx_busy()):eq(false)
    t:expect(t:expr(u):is_readable()):eq(true)
    t:expect(t:expr(u):read_blocking(#data)):eq(data)
    t:expect(t:expr(u):is_readable()):eq(false)
end

function test_threaded_write_read(t)
    local u = setup(t)
    local cnt, data = 100, "0123456"
    local writer<close> = thread.start(function()
        for i = 1, cnt do u:write_blocking(data) end
    end)
    for i = 1, cnt do
        t:expect(t:expr(u):read_blocking(#data)):eq(data)
    end
    t:expect(t:expr(u):is_readable()):eq(false)
end
