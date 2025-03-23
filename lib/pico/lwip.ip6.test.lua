-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local lwip = require 'lwip'
local ip6 = require 'lwip.ip6'
local netif = require 'lwip.netif'
local table = require 'table'

function test_addresses(t)
    for _, test in ipairs{
        {'ANY', '::'},
        {'LOOPBACK', '::1'},
    } do
        local addr, want = table.unpack(test)
        t:expect(t.expr(ip6)[addr]):eq(lwip.ipaddr_aton(want))
    end
end

local addr_meth = {'any', 'broadcast', 'multicast', 'loopback', 'linklocal'}
local addr_fn = {'global', 'sitelocal', 'uniquelocal', 'ipv4_mapped_ipv6',
                 'ipv4_compat'}

function test_IPAddr(t)
    local lo = netif.find('lo0')
    for _, test in ipairs{
        {ip6.ANY, any = true},
        {ip6.LOOPBACK, loopback = true},
        {'ff02::1234', multicast = true},
        {'fe80::1234', linklocal = true},
        {'2001:4860:4860::8888', global = true},
        {'fec0::1234', sitelocal = true},
        {'fd12::3456', uniquelocal = true},
        {'::ffff:12.34.56.78', ipv4_mapped_ipv6 = true},
        {'::0:12.34.56.78', ipv4_compat = true},
    } do
        local addr = table.unpack(test)
        if type(addr) == 'string' then addr = lwip.ipaddr_aton(addr) end
        for _, fn in ipairs(addr_meth) do
            local args = {addr}
            if fn == 'broadcast' then table.insert(args, lo) end
            t:expect(t.expr(addr)['is_' .. fn](table.unpack(args)))
                :eq(test[fn] or false)
        end
        for _, fn in ipairs(addr_fn) do
            t:expect(t.expr(ip6)['is_' .. fn](addr)):eq(test[fn] or false)
        end
    end
end

function test_multicast_scope(t)
    for _, test in ipairs{
        {'ff01::', ip6.MULTICAST_SCOPE_INTERFACE_LOCAL},
        {'ff02::', ip6.MULTICAST_SCOPE_LINK_LOCAL},
        {'ff03::', ip6.MULTICAST_SCOPE_RESERVED3},
        {'ff04::', ip6.MULTICAST_SCOPE_ADMIN_LOCAL},
        {'ff05::', ip6.MULTICAST_SCOPE_SITE_LOCAL},
        {'ff08::', ip6.MULTICAST_SCOPE_ORGANIZATION_LOCAL},
        {'ff0e::', ip6.MULTICAST_SCOPE_GLOBAL},
    } do
        local addr, want = table.unpack(test)
        t:expect(t.expr(ip6).multicast_scope(lwip.ipaddr_aton(addr))):eq(want)
    end
end
