-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local lwip = require 'lwip'
local ip4 = require 'lwip.ip4'
local ip6 = require 'lwip.ip6'
local netif = require 'lwip.netif'
local io = require 'mlua.io'
local table = require 'table'

local function hw_addr(addr)
    local first = true
    return addr:gsub('.', function(c)
        local res = ('%s%02x'):format(first and '' or ':', c:byte())
        first = false
        return res
    end)
end

local function addr_scope(ip)
    if ip6.is_global(ip) then return 'global' end
    if ip:is_linklocal() then return 'link' end
    if ip6.is_sitelocal(ip) then return 'site' end
    if ip:is_loopback() then return 'host' end
    return 'unknown'
end

local function addr_state(state)
    if state & ip6.ADDR_TENTATIVE ~= 0 then return 'tentative' end
    if state == ip6.ADDR_PREFERRED then return 'preferred' end
    if state == ip6.ADDR_DEPRECATED then return 'deprecated' end
    if state == ip6.ADDR_DUPLICATED then return 'duplicated' end
    return 'unknown'
end

local function addr_life(v)
    if v == ip6.ADDR_LIFE_STATIC or v == ip6.ADDR_LIFE_INFINITE then
        return 'forever'
    end
    return ('%ssec'):format(v)
end

function set_up(t)
    for nif in netif.iter() do
        local flags, fnames = nif:flags(), {}
        for _, f in ipairs{'UP', 'LINK_UP', 'BROADCAST', 'ETHARP', 'ETHERNET',
                           'IGMP', 'MLD6'} do
            if flags & netif['FLAG_' .. f] ~= 0 then table.insert(fnames, f) end
        end
        local mtu, mtu6 = nif:mtu()
        t:printf("%s: @{+CYAN}%s:@{NORM} <%s> mtu %s mtu6 %s state %s\n",
                 nif:index(), nif:name(), table.concat(fnames, ','), mtu, mtu6,
                 io.ansi(flags & netif.FLAG_UP ~= 0 and '@{+GREEN}UP@{NORM}' or
                         '@{+RED}DOWN@{NORM}'))
        local hwaddr = nif:hwaddr()
        if hwaddr ~= '' then
            t:printf("    link @{+YELLOW}%s@{NORM}\n", hw_addr(hwaddr))
        end
        local ip, mask, gw = nif:ip4()
        if ip then
            t:printf("    inet @{+MAGENTA}%s@{NORM}/%s gw %s\n", ip, mask, gw)
        end
        for _, ip, state, valid, pref in nif:ip6() do
            if state ~= ip6.ADDR_INVALID then
                local scope = ip6.multicast_scope(ip)
                t:printf("    inet6 @{+BLUE}%s@{NORM} scope %s %s\n", ip,
                         addr_scope(ip), addr_state(state))
                if valid and pref then
                    t:printf("        valid_lft %s preferred_lft %s\n",
                             addr_life(valid), addr_life(pref))
                end
            end
        end
    end
end

function test_loopback(t)
    local lo = netif.find('lo0')
    if not lo then t:skip('no loopback netif') end
    t:expect(t.expr(lo):ip4()):eq(ip4.LOOPBACK);
    t:expect(t.expr(lo):ip6(0)):eq(ip6.LOOPBACK);
end
