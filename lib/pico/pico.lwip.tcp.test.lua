-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local mem = require 'mlua.mem'
local testing_lwip = require 'mlua.testing.lwip'
local thread = require 'mlua.thread'
local group = require 'mlua.thread.group'
local time = require 'mlua.time'
local lwip = require 'pico.lwip'
local tcp = require 'pico.lwip.tcp'
local table = require 'table'

function test_accessors(t)
    local sock<close> = lwip.assert(tcp.new())
    t:expect(t.expr(sock):options(lwip.SOF_REUSEADDR | lwip.SOF_KEEPALIVE))
        :eq(0)
    t:expect(t.expr(sock):options(nil, lwip.SOF_REUSEADDR))
        :eq(lwip.SOF_REUSEADDR | lwip.SOF_KEEPALIVE)
    t:expect(t.expr(sock):options()):eq(lwip.SOF_KEEPALIVE)
    t:expect(t.expr(sock):options()):eq(lwip.SOF_KEEPALIVE)
    t:expect(t.expr(sock):tos(12)):eq(0)
    t:expect(t.expr(sock):tos()):eq(12)
    t:expect(t.expr(sock):tos()):eq(12)
    t:expect(t.expr(sock):ttl(127)):eq(255)
    t:expect(t.expr(sock):ttl()):eq(127)
    t:expect(t.expr(sock):ttl()):eq(127)
    t:expect(t.expr(sock):prio(tcp.PRIO_MAX)):eq(tcp.PRIO_NORMAL)
    t:expect(t.expr(sock):prio()):eq(tcp.PRIO_MAX)
    t:expect(t.expr(sock):prio()):eq(tcp.PRIO_MAX)
end

function test_listen(t)
    for _, test in ipairs{
        {"small", nil, {1234}, 1, 64, 10, 10 * time.msec},
        {"large", nil, {1234}, 1, 1200, 10, 50 * time.msec},
        {"many", nil, {1234}, 1, 64, 100, 10 * time.msec},
        {"fast", nil, {1234}, 1, 8, 10, 1 * time.usec},
        {"multi-conn", nil, {1234}, 3, 64, 10, 10 * time.msec},
        -- {"multi-port", nil, {1234, 5678, 910}, 1, 64, 10, 10 * time.msec},
        {"IPv4", testing_lwip.IPV4, {1234}, 1, 64, 10, 10 * time.msec},
        {"IPv6", testing_lwip.IPV6, {1234}, 1, 64, 10, 10 * time.msec},
    } do
        local desc, atype, ports = table.unpack(test, 1, 3)
        t:run(desc, function(t)
            local tg<close> = thread.Group()
            for _, lport in ipairs(ports) do
                tg:start(log_error(function()
                    return run_listen_test(t, atype, lport,
                                           table.unpack(test, 4))
                end))
            end
        end)
    end
end

function run_listen_test(t, atype, lport, conns, size, count, interval)
    local ctrl = testing_lwip.Control(t, atype)
    local sock<close> = lwip.assert(tcp.new())
    lwip.assert(sock:bind(nil, lport))
    t:expect(t.expr(sock):local_ip()):eq(lwip.IP_ANY_TYPE)
    t:expect(t.expr(sock):local_port()):eq(lport)
    lwip.assert(sock:listen())

    local dl = time.deadline(count * interval + 1000 * time.msec)
    local workers<close> = thread.Group()
    local accepter<close> = thread.start(log_error(function()
        while true do
            local conn = lwip.assert(sock:accept(dl))
            t:expect(t.expr(conn):remote_ip()):eq(ctrl.addr)
            workers:start(log_error(function()
                local conn<close> = conn
                local received = testing_lwip.Set()
                while true do
                    local dsize = lwip.assert(conn:recv(2, dl))
                    if #dsize < 2 then break end
                    local size = dsize:byte(1) | (dsize:byte(2) << 8)
                    local data = lwip.assert(conn:recv(size, dl))
                    if #data < size then break end
                    local pid, ok = testing_lwip.verify_data(data, 0, #data)
                    t:expect(ok):label("data ok"):eq(true)
                    lwip.assert(conn:send(dsize, data, dl))
                    if received:add(pid) == count then break end
                end
            end))
        end
    end))

    local received = testing_lwip.Set()
    local receiver<close> = thread.start(log_error(function()
        while true do
            local line = ctrl:recv(dl)
            if line == '' then break end
            if line == 'DONE\n' then break end
            local rsize, pid, ok = line:match('^RECV (%d+) (%d+) (.+)\n$')
            t:expect(tonumber(rsize)):label("rsize"):eq(size)
            t:expect(ok):label("data ok"):eq('OK')
            if received:add(pid) == conns * count then break end
        end
    end))

    ctrl:send(dl, 'tcp_connect %s %s %s %s %s\n', lport, conns, size, count,
              interval)
    receiver:join()
    t:expect(#received):label("received"):eq(conns * count)
    accepter:kill()
end

function test_send(t)
    for _, test in ipairs{
        {"small", nil, {1234}, 64, 10, 10 * time.msec},
        {"large", nil, {1234}, 1200, 10, 50 * time.msec},
        {"many", nil, {1234}, 64, 100, 10 * time.msec},
        {"fast", nil, {1234}, 8, 10, 1 * time.usec},
        {"multi-port", nil, {1234, 5678, 910}, 64, 10, 10 * time.msec},
        {"IPv4", testing_lwip.IPV4, {1234}, 64, 10, 10 * time.msec},
        {"IPv6", testing_lwip.IPV6, {1234}, 64, 10, 10 * time.msec},
    } do
        local desc, atype, ports = table.unpack(test, 1, 3)
        t:run(desc, function(t)
            local tg<close> = thread.Group()
            for _, lport in ipairs(ports) do
                tg:start(log_error(function()
                    return run_send_test(t, atype, lport, table.unpack(test, 4))
                end))
            end
        end)
    end
end

function run_send_test(t, atype, lport, size, count, interval)
    local ctrl = testing_lwip.Control(t, atype)
    local sock<close> = lwip.assert(tcp.new())
    lwip.assert(sock:bind(nil, lport))
    t:expect(t.expr(sock):local_ip()):eq(lwip.IP_ANY_TYPE)
    t:expect(t.expr(sock):local_port()):eq(lport)

    local dl = time.deadline(count * interval + 1000 * time.msec)
    ctrl:send(dl, 'tcp_listen %s 0 0 0\n', lport)
    t:assert(ctrl:recv(dl)):label("listening"):eq('LISTENING\n')

    local received = testing_lwip.Set()
    local receiver<close> = thread.start(log_error(function()
        while true do
            local line = ctrl:recv(dl)
            if line == '' then break end
            local rsize, pid, ok = line:match('^RECV (%d+) (%d+) (.+)\n$')
            t:expect(tonumber(rsize)):label("rsize"):eq(size)
            t:expect(ok):label("data ok"):eq('OK')
            if received:add(pid) == count then break end
        end
    end))

    lwip.assert(sock:connect(ctrl.addr, ctrl.port))
    t:expect(t.expr(sock):remote_ip()):eq(ctrl.addr)
    t:expect(t.expr(sock):remote_port()):eq(ctrl.port)

    local sender<close> = thread.start(log_error(function()
        local p = mem.alloc(2 + size)
        mem.set(p, 0, size & 0xff, (size >> 8) & 0xff)
        for pid = 0, count - 1 do
            testing_lwip.generate_data(pid, size, p, 2)
            lwip.assert(sock:send(p, dl))
            time.sleep_for(interval)
        end
    end))

    receiver:join()
    t:expect(#received):label("received"):eq(count)
end

function test_recv(t)
    for _, test in ipairs{
        {"small", nil, {1234}, 64, 10, 10 * time.msec},
        {"large", nil, {1234}, 1200, 10, 10 * time.msec},
        {"many", nil, {1234}, 64, 100, 10 * time.msec},
        {"fast", nil, {1234}, 64, 10, 1 * time.usec},
        {"multi-port", nil, {1234, 5678, 910}, 64, 10, 10 * time.msec},
        {"IPv4", testing_lwip.IPV4, {1234}, 64, 10, 10 * time.msec},
        {"IPv6", testing_lwip.IPV6, {1234}, 64, 10, 10 * time.msec},
    } do
        local desc, atype, ports = table.unpack(test, 1, 3)
        t:run(desc, function(t)
            local tg<close> = thread.Group()
            for _, lport in ipairs(ports) do
                tg:start(log_error(function()
                    return run_recv_test(t, atype, lport, table.unpack(test, 4))
                end))
            end
        end)
    end
end

function run_recv_test(t, atype, lport, size, count, interval)
    local ctrl = testing_lwip.Control(t, atype)
    local sock<close> = lwip.assert(tcp.new())
    lwip.assert(sock:bind(nil, lport))

    local dl = time.deadline(count * interval + 1000 * time.msec)
    ctrl:send(dl, 'tcp_listen %s %s %s %s\n', lport, size, count, interval)
    t:assert(ctrl:recv(dl)):label("listening"):eq('LISTENING\n')

    lwip.assert(sock:connect(ctrl.addr, ctrl.port))

    local received = testing_lwip.Set()
    local receiver<close> = thread.start(log_error(function()
        while true do
            local dsize = lwip.assert(sock:recv(2, dl))
            if #dsize < 2 then break end
            local size = dsize:byte(1) | (dsize:byte(2) << 8)
            local data = lwip.assert(sock:recv(size, dl))
            if #data < size then break end
            local pid, ok = testing_lwip.verify_data(data, 0, #data)
            t:expect(ok):label("data ok"):eq(true)
            if received:add(pid) == count then break end
        end
    end))

    receiver:join()
    t:expect(#received):label("received"):eq(count)
end
