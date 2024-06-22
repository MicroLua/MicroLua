-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local io = require 'mlua.io'
local testing_lwip = require 'mlua.testing.lwip'
local thread = require 'mlua.thread'
local group = require 'mlua.thread.group'
local time = require 'mlua.time'
local lwip = require 'pico.lwip'
local udp = require 'pico.lwip.udp'
local table = require 'table'

function test_recv(t)
    for _, test in ipairs{
        {'recv', "small packets", {1234}, 64, 10, 10 * time.msec},
        {'recvfrom', "small packets", {1234}, 64, 10, 10 * time.msec},
        {'recvfrom', "large packets", {1234}, 2000, 10, 10 * time.msec},
        {'recvfrom', "many packets", {1234}, 64, 100, 10 * time.msec},
        {'recvfrom', "multiple ports",
            {1234, 5678, 910}, 64, 10, 10 * time.msec},
        -- TODO: Add a test with IPv6
    } do
        t:run(('%s: %s'):format(test[1], test[2]), function(t)
            local tg<close> = thread.Group()
            for _, lport in ipairs(test[3]) do
                tg:start(with_traceback(function()
                    return run_recv_test(t, test[1], lport,
                                         table.unpack(test, 4))
                end))
            end
        end)
    end
end

function run_recv_test(t, fname, lport, size, count, interval)
    local ctrl = testing_lwip.Control(t)
    local sock<close> = udp.new(nil, count)
    sock:bind(lwip.IP_ANY_TYPE, lport)
    sock:connect(ctrl.addr, ctrl.port)

    local recv, from = sock[fname], fname == 'recvfrom'
    local received = testing_lwip.Set()
    local recveiver<close> = thread.start(with_traceback(function()
        local dl = time.deadline(count * interval + 500 * time.msec)
        while true do
            local p<close>, addr, port = recv(sock, dl)
            if not p then break end
            t:expect(addr):label("addr"):eq(from and ctrl.addr or nil)
            t:expect(port):label("port"):eq(from and ctrl.port or nil)
            local pid, ok = testing_lwip.verify_data(p, 0, #p)
            t:expect(ok):label("data ok"):eq(true)
            if received:add(pid) == count then break end
        end
    end))

    ctrl:send('udp_send %s %s %s %s\n', lport, size, count, interval)
    ctrl:wait_close(t)
    recveiver:join()
    t:expect(#received):label("received"):eq(count)
end
