_ENV = mlua.Module(...)

local list = require 'mlua.list'
local testing_stdio = require 'mlua.testing.stdio'
local thread = require 'mlua.thread'
local util = require 'mlua.util'
local stdio = require 'pico.stdio'
local string = require 'string'

-- We don't test the pico.stdio.semihosting module, because it requires a
-- debugger to be attached. Attempts to catch bkpt instructions through the
-- hardfault handler have failed; the handler doesn't seem to be called, and
-- the core just locks up.

-- We don't test the pico.stdio.usb module, because it sometimes causes the
-- xhci_hcd driver to lock up and terminate, thereby disconnecting all USB
-- devices and requiring a reboot.

function test_putchar_getchar_Y(t)
    for _, test in ipairs{
        {false, {65, 10, 66, 13, 10, 67},
                {65, 10, 66, 13, 10, 67, 65, 10, 66, 13, 10, 67, -1}},
        {true, {65, 10, 66, 13, 10, 67},
               {65, 10, 66, 13, 10, 67, 65, 13, 10, 66, 13, 10, 67, -1}},
    } do
        local crlf, chars, want = list.unpack(test)
        local got = list()
        t:expect(pcall(function()
            local done<close> = testing_stdio.enable_loopback(t, crlf)
            for _, c in ipairs(chars) do stdio.putchar_raw(c) end
            for _, c in ipairs(chars) do stdio.putchar(c) end
            for i = 1, 4 do got:append(stdio.getchar()) end
            for i = 5, #want do got:append(stdio.getchar_timeout_us(1000)) end
        end))
        t:expect(got):label("crlf: %s, got", crlf):eq(want, list.eq)
    end
end

function test_puts(t)
    for _, test in ipairs{
        {false, 'ab\ncd\r\nef', 'ab\ncd\r\nef\nab\ncd\r\nef\n'},
        {true, 'ab\ncd\r\nef', 'ab\ncd\r\nef\nab\r\ncd\r\nef\r\n'},
    } do
        local crlf, s, want = list.unpack(test)
        local got = ''
        t:expect(pcall(function()
            local done<close> = testing_stdio.enable_loopback(t, crlf)
            stdio.puts_raw(s)
            stdio.puts(s)
            for i = 1, #want do got = got .. string.char(stdio.getchar()) end
        end))
        t:expect(got):label("crlf: %s, got", crlf):eq(want)
    end
end

function test_write_read_Y(t)
    for _, test in ipairs{
        {false, {'a', '', 'bc\n', 'def', 'gh\nij'}, 'abc\ndefgh\nij'},
        {true, {'a', '', 'bc\n', 'def', 'gh\nij'}, 'abc\r\ndefgh\r\nij'},
    } do
        local crlf, writes, want = list.unpack(test)
        local wr, got = list(), ''
        t:expect(pcall(function()
            local done<close> = testing_stdio.enable_loopback(t, crlf)
            for _, w in ipairs(writes) do wr:append(stdio.write(w)) end
            while #got < #want do got = got .. stdio.read(#want - #got) end
        end))
        for i, w in ipairs(writes) do
            t:expect(wr[i]):label("write(%s)", util.repr(w)):eq(#w)
        end
        t:expect(got):label("crlf: %s, got", crlf):eq(want)
    end
end

function test_chars_available(t)
    local got, want = '', 'abcdefghijklmnopqrstuvwxyz'
    t:expect(pcall(function()
        local done<close> = testing_stdio.enable_loopback(t, false)
        local reader<close> = thread.start(function()
            while true do
                stdio.wait_chars_available()
                local c = stdio.getchar()
                if c == 10 then return end
                got = got .. string.char(c)
            end
        end)
        stdio.puts_raw(want)
    end))
    t:expect(got):label("got"):eq(want)
end
