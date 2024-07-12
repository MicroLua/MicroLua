-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local time = require 'mlua.time'
local lwip = require 'pico.lwip'
local dns = require 'pico.lwip.dns'
local table = require 'table'

function set_up(t)
    for i = 0, dns.MAX_SERVERS - 1 do
        local addr = dns.getserver(i)
        if addr then t:printf("DNS server: %s\n", addr) end
    end
end

function test_gethostbyname(t)
    for _, test in ipairs{
        {'dns.google', lwip.IPV4 and dns.ADDRTYPE_IPV4 or false,
         {'8.8.8.8', '8.8.4.4'}},
        {'dns.google', lwip.IPV6 and dns.ADDRTYPE_IPV6 or false,
         {'2001:4860:4860::8888', '2001:4860:4860::8844'}},
        {'this.is.invalid', nil, {false}},
    } do
        local host, typ, want = table.unpack(test)
        if typ == false then goto continue end
        local addr, err = dns.gethostbyname(host, typ,
                                            time.deadline(5 * time.sec))
        t:expect(err == nil, "DNS request failed: %s",
                 function() return lwip.err_str(err) end)
        if err ~= nil then goto continue end
        for i, a in ipairs(want) do
            if type(a) == 'string' then want[i] = lwip.ipaddr_aton(a) end
        end
        t:expect(addr):label("addr"):eq_one_of(want)
        ::continue::
    end
end
