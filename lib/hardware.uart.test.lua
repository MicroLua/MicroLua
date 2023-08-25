_ENV = mlua.Module(...)

local uart = require 'hardware.uart'
local math = require 'math'
local list = require 'mlua.list'
local thread = require 'mlua.thread'
local time = require 'pico.time'
local string = require 'string'

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
    local baud = 1000000
    t:expect(t:expr(u):init(baud)):close_to_rel(baud, 0.05)
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
    local baud = 460800
    t:expect(t:expr(u):set_baudrate(baud)):close_to_rel(baud, 0.05)
    u:set_format(7, 2, uart.PARITY_ODD)
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
    local start = time.get_absolute_time()
    local got = u:is_readable_within_us(1000)
    local now = time.get_absolute_time()
    t:expect(got):label("is_readable_within_us()"):eq(false)
    t:expect(now - start):label("is_readable_within_us() duration"):gte(1000)

    u:write_blocking(data)
    local busy = u:is_tx_busy()  -- t:expr() is too slow for high bitrates
    t:expect(busy):label("is_tx_busy()"):eq(true)
    u:tx_wait_blocking()
    t:expect(t:expr(u):is_tx_busy()):eq(false)

    t:expect(t:expr(u):is_readable()):eq(true)
    local start = time.get_absolute_time()
    local got = u:is_readable_within_us(1000000)
    local now = time.get_absolute_time()
    t:expect(got):label("is_readable_within_us()"):eq(true)
    t:expect(now - start):label("is_readable_within_us() duration"):lt(100)

    t:expect(t:expr(u):read_blocking(#data)):eq(data)
    t:expect(t:expr(u):is_readable()):eq(false)
end

function test_threaded_write_read(t)
    local u = setup(t)
    local cnt, data = 50, "0123456"
    local writer<close> = thread.start(function()
        for i = 1, cnt do
            u:write_blocking(data)
            thread.yield()
        end
    end)
    for i = 1, cnt do
        local got = u:read_blocking(#data)  -- t:expr() is slow
        t:expect(got):label("got"):eq(data)
    end
    t:expect(t:expr(u):is_readable()):eq(false)
end

function test_putc_getc_Y(t)
    local u = setup(t)
    for _, test in ipairs{
        {false, {65, 10, 66, 13, 10, 67},
                {65, 10, 66, 13, 10, 67, 65, 10, 66, 13, 10, 67}},
        -- BUG(pico-sdk): A CR/LF in the input should output a CR/LF, not a
        --   CR/CR/LF. pico_stdio does it correctly. There's a similar issue
        --   across two puts() calls, with the first ending with a CR and the
        --   second starting with an LF.
        {true, {65, 10, 66, 13, 10, 67},
               {65, 10, 66, 13, 10, 67, 65, 13, 10, 66, 13, 13, 10, 67}},
    } do
        local crlf, chars, want = list.unpack(test)
        u:set_translate_crlf(crlf)
        for _, c in ipairs(chars) do u:putc_raw(c) end
        for _, c in ipairs(chars) do u:putc(c) end
        local got = list()
        for i = 1, #want do got:append(u:getc()) end
        t:expect(got):label("crlf: %s, got", crlf):eq(want, list.eq)
    end
end

function test_puts(t)
    local u = setup(t)
    for _, test in ipairs{
        {false, 'ab\ncd\r\nef', 'ab\ncd\r\nef'},
        {true, 'ab\ncd\r\nef', 'ab\r\ncd\r\nef'},
    } do
        local crlf, s, want = list.unpack(test)
        u:set_translate_crlf(crlf)
        u:puts(s)
        local got = ''
        for i = 1, #want do got = got .. string.char(u:getc()) end
        t:expect(got):label("crlf: %s, got", crlf):eq(want)
    end
end
