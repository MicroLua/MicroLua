-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local lwip = require 'lwip'
local pbuf = require 'lwip.pbuf'
local udp = require 'lwip.udp'
local testing_lwip = require 'mlua.testing.lwip'
local thread = require 'mlua.thread'
local group = require 'mlua.thread.group'
local time = require 'mlua.time'
local table = require 'table'

function test_accessors(t)
    local sock<close> = lwip.assert(udp.new(nil, 0))
    t:expect(t.expr(sock):options(lwip.SOF_REUSEADDR | lwip.SOF_BROADCAST))
        :eq(0)
    t:expect(t.expr(sock):options(nil, lwip.SOF_REUSEADDR))
        :eq(lwip.SOF_REUSEADDR | lwip.SOF_BROADCAST)
    t:expect(t.expr(sock):options()):eq(lwip.SOF_BROADCAST)
    t:expect(t.expr(sock):options()):eq(lwip.SOF_BROADCAST)
    t:expect(t.expr(sock):tos(12)):eq(0)
    t:expect(t.expr(sock):tos()):eq(12)
    t:expect(t.expr(sock):tos()):eq(12)
    t:expect(t.expr(sock):ttl(127)):eq(255)
    t:expect(t.expr(sock):ttl()):eq(127)
    t:expect(t.expr(sock):ttl()):eq(127)
end

function test_send(t)
    for _, test in ipairs{
        {"send", 'send', nil, {1234}, 64, 10, 10 * time.msec},
        {"small", 'sendto', nil, {1234}, 64, 10, 10 * time.msec},
        {"large", 'sendto', nil, {1234}, 1200, 10, 50 * time.msec},
        {"many", 'sendto', nil, {1234}, 64, 100, 5 * time.msec},
        {"fast", 'sendto', nil, {1234}, 8, 10, 1 * time.usec},
        {"multi", 'sendto', nil, {1234, 5678, 910}, 64, 10, 10 * time.msec},
        {"IPv4", 'sendto', testing_lwip.IPV4, {1234}, 64, 10, 10 * time.msec},
        {"IPv6", 'sendto', testing_lwip.IPV6, {1234}, 64, 10, 10 * time.msec},
    } do
        local desc, fname, atype, ports, size, count, interval =
            table.unpack(test, 1, 7)
        t:run(desc, function(t)
            local tg<close> = thread.Group()
            for _, lport in ipairs(ports) do
                tg:start(log_error(function()
                    return run_send_test(t, fname, atype, lport, size, count,
                                         interval)
                end))
            end
        end)
    end
end

function run_send_test(t, fname, atype, lport, size, count, interval)
    local ctrl = testing_lwip.Control(t, atype)
    local sock<close> = lwip.assert(udp.new(nil, count))
    lwip.assert(sock:bind(nil, lport))
    t:expect(t.expr(sock):local_ip()):eq(lwip.IP_ANY_TYPE)
    t:expect(t.expr(sock):local_port()):eq(lport)
    lwip.assert(sock:connect(ctrl.addr, ctrl.port))
    t:expect(t.expr(sock):remote_ip()):eq(ctrl.addr)
    t:expect(t.expr(sock):remote_port()):eq(ctrl.port)

    local dl = time.deadline(count * interval + 1000 * time.msec)
    ctrl:send(dl, 'udp %s 0 0 0\n', lport)
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

    local send, to = sock[fname], fname == 'sendto'
    local sender<close> = thread.start(log_error(function()
        for pid = 0, count - 1 do
            local p<close> = lwip.assert(pbuf.alloc(pbuf.TRANSPORT, size))
            testing_lwip.generate_data(pid, size, p, 0)
            local addr = to and ctrl.addr or nil
            local port = to and ctrl.port or nil
            local ok, err = send(sock, p, addr, port)
            t:expect(ok):label("send() ok"):eq(true)
            t:expect(err):label("send() err"):eq(nil)
            time.sleep_for(interval)
        end
    end))

    receiver:join()
    t:expect(#received):label("received"):eq(count)
end

function test_recv(t)
    for _, test in ipairs{
        {"recv", 'recv', nil, {1234}, 64, 10, 10 * time.msec},
        {"small", 'recvfrom', nil, {1234}, 64, 10, 10 * time.msec},
        {"large", 'recvfrom', nil, {1234}, 1200, 10, 10 * time.msec},
        {"many", 'recvfrom', nil, {1234}, 64, 100, 5 * time.msec},
        {"fast", 'recvfrom', nil, {1234}, 64, 10, 1 * time.usec},
        {"multi", 'recvfrom', nil, {1234, 5678, 910}, 64, 10, 10 * time.msec},
        {"IPv4", 'recvfrom', testing_lwip.IPV4, {1234}, 64, 10, 10 * time.msec},
        {"IPv6", 'recvfrom', testing_lwip.IPV6, {1234}, 64, 10, 10 * time.msec},
    } do
        local desc, fname, atype, ports, size, count, interval =
            table.unpack(test, 1, 7)
        t:run(desc, function(t)
            local tg<close> = thread.Group()
            for _, lport in ipairs(ports) do
                tg:start(log_error(function()
                    return run_recv_test(t, fname, atype, lport, size, count,
                                         interval)
                end))
            end
        end)
    end
end

function run_recv_test(t, fname, atype, lport, size, count, interval)
    local ctrl = testing_lwip.Control(t, atype)
    local sock<close> = lwip.assert(udp.new(nil, count))
    lwip.assert(sock:bind(nil, lport))
    lwip.assert(sock:connect(ctrl.addr, ctrl.port))

    local dl = time.deadline(count * interval + 1000 * time.msec)
    local recv, from = sock[fname], fname == 'recvfrom'
    local received = testing_lwip.Set()
    local receiver<close> = thread.start(log_error(function()
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

    ctrl:send(dl, 'udp %s %s %s %s\n', lport, size, count, interval)

    receiver:join()
    t:expect(#received):label("received"):eq(count)
end
