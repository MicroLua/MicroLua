-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local lwip = require 'lwip'
local dns = require 'lwip.dns'
local time = require 'mlua.time'
local table = require 'table'

function set_up(t)
    local servers = {}
    for i = 0, dns.MAX_SERVERS - 1 do
        local addr = dns.getserver(i)
        if addr then table.insert(servers, tostring(addr)) end
    end
    t:printf("DNS servers: %s\n", table.concat(servers, ' '))
end

function test_gethostbyname(t)
    for _, test in ipairs{
        {'dns.google', dns.ADDRTYPE_IPV4, {'8.8.8.8', '8.8.4.4'}},
        {'dns.google', dns.ADDRTYPE_IPV6,
         {'2001:4860:4860::8888', '2001:4860:4860::8844'}},
        {'this.is.invalid', nil, {false}},
    } do
        local host, typ, want = table.unpack(test, 1, 3)
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
