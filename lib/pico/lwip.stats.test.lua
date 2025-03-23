-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local stats = require 'lwip.stats'

function test_proto_stats(t)
    for _, ss in ipairs{'link', 'etharp', 'ip_frag', 'ip', 'icmp', 'udp', 'tcp',
                        'ip6', 'icmp6', 'ip6_frag', 'nd6'} do
        t:expect(t.expr(stats).SUBSYSTEMS):has(ss)
        local fn = stats[ss]
        if fn ~= false then
            t:expect(fn()):label(ss)
                :has('xmit'):has('recv'):has('drop'):has('err')
        end
    end
end

function test_igmp_stats(t)
    for _, ss in ipairs{'igmp', 'mld6'} do
        t:expect(t.expr(stats).SUBSYSTEMS):has(ss)
        local fn = stats[ss]
        if fn ~= false then
            t:expect(fn()):label(ss)
                :has('xmit'):has('recv'):has('drop'):has('proterr')
        end
    end
end

function test_mem_stats(t)
    for _, ss in ipairs{'mem', 'memp'} do
        t:expect(t.expr(stats).SUBSYSTEMS):has(ss)
        local fn = stats[ss]
        if fn ~= false then
            t:expect((fn(0))):label(ss):has('avail'):has('used'):has('max')
        end
    end
end

function test_mib2_stats(t)
    t:expect(t.expr(stats).SUBSYSTEMS):has('mib2')
    local fn = stats.mib2
    if fn == false then return end
    t:expect(fn()):label("stats")
        :has('ipinhdrerrors'):has('tcpactiveopens'):has('icmpinechos')
end
