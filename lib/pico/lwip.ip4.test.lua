-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local lwip = require 'lwip'
local ip4 = require 'lwip.ip4'
local netif = require 'lwip.netif'
local table = require 'table'

function test_addresses(t)
    for _, test in ipairs{
        {'ANY', '0.0.0.0'},
        {'LOOPBACK', '127.0.0.1'},
        {'BROADCAST', '255.255.255.255'},
    } do
        local addr, want = table.unpack(test)
        t:expect(t.expr(ip4)[addr]):eq(lwip.ipaddr_aton(want))
    end
end

function test_network(t)
    t:expect(t.expr(ip4).network(lwip.ipaddr_aton('172.16.123.45'),
                                 lwip.ipaddr_aton('255.255.255.0')))
        :eq(lwip.ipaddr_aton('172.16.123.0'))
end

local addr_meth = {'any', 'broadcast', 'multicast', 'loopback', 'linklocal'}

function test_IPAddr(t)
    local lo = netif.find('lo0')
    for _, test in ipairs{
        {ip4.ANY, any = true, broadcast = true},
        {ip4.LOOPBACK, loopback = true},
        {ip4.BROADCAST, broadcast = true},
        {'224.0.0.1', multicast = true},
        {'169.254.1.2', linklocal = true},
    } do
        local addr = table.unpack(test)
        if type(addr) == 'string' then addr = lwip.ipaddr_aton(addr) end
        t:expect(t.expr(addr):type()):eq(lwip.IPADDR_TYPE_V4)
        for _, fn in ipairs(addr_meth) do
            local args = {addr}
            if fn == 'broadcast' then table.insert(args, lo) end
            t:expect(t.expr(addr)['is_' .. fn](table.unpack(args)))
                :eq(test[fn] or false)
        end
    end
end
