-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local lwip = require 'lwip'

function set_up(t)
    t:printf("lwIP: %s, IPv4: %s, IPv6: %s\n", lwip.VERSION_STRING, lwip.IPV4,
             lwip.IPV6)
end

function test_err_str(t)
    t:expect(t.expr(lwip).err_str(lwip.ERR_OK)):eq("no error")
    t:expect(t.expr(lwip).err_str(lwip.ERR_MEM)):eq("out of memory")
end

function test_assert(t)
    t:expect(t.mexpr(lwip).assert(1, 2, 3)):eq{1, 2, 3}
    t:expect(t.expr(lwip).assert(false, lwip.ERR_MEM)):raises("out of memory")
    t:expect(t.expr(lwip).assert(false, "boom")):raises("boom")
end

function test_IPAddr(t)
    t:expect(t.expr(lwip).IP_ANY_TYPE:type()):eq(lwip.IPADDR_TYPE_ANY)
end
