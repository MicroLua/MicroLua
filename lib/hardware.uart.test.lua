_ENV = mlua.Module(...)

local uart = require 'hardware.uart'
local thread = require 'mlua.thread'

function test_strict(t)
    local ok = pcall(function() return uart.UNKNOWN end)
    t:expect(not ok, "module is non-strict")
    local ok = pcall(function() return uart[0].UNKNOWN end)
    t:expect(not ok, "Uart instance is non-strict")
end

function test_blocking_write_read_Y(t)
    local u = uart[1]
    local got = u:get_index()
    t:expect(got == 1, "get_index() = %s, want 1", got)
    u:init(115200)
    t:cleanup(function() u:deinit() end)
    u:set_format(8, 1, uart.PARITY_NONE)
    u:set_hw_flow(false, false)
    u:set_fifo_enabled(true)
    u:enable_loopback(true)
    u:enable_irq()
    local got = u:is_enabled()
    t:expect(got == true, "is_enabled() = %s, want true", got)
    local got = u:is_writable()
    t:expect(got == true, "is_writable() = %s, want true", got)
    local got = u:is_readable()
    t:expect(got == false, "is_readable() = %s, want false", got)
    local data = "0123456789abcdefghijklmnopqrstuv"  -- Fits the FIFO
    u:write_blocking(data)
    u:tx_wait_blocking()
    local got = u:is_readable()
    t:expect(got == true, "is_readable() = %s, want true", got)
    local got = u:read_blocking(#data)
    t:expect(got == data, "Unexpected data: got %q, want %q", got, data)
end

function test_threaded_write_read(t)
    local u = uart[1]
    u:init(115200)
    t:cleanup(function() u:deinit() end)
    u:enable_loopback(true)
    u:enable_irq()
    local cnt, data = 100, "0123456"
    local writer<close> = thread.start(function()
        for i = 1, cnt do u:write_blocking(data) end
    end)
    for i = 1, cnt do
        local got = u:read_blocking(#data)
        t:expect(got == data, "Unexpected data: got %q, want %q", got, data)
    end
    local got = u:is_readable()
    t:expect(got == false, "is_readable() = %s, want false", got)
end
