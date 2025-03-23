-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local pbuf = require 'lwip.pbuf'
local mem = require 'mlua.mem'

function test_PBUF(t)
    local p<close> = pbuf.alloc(pbuf.TRANSPORT, 10)
    t:expect(#p):label("#p"):eq(10)
    mem.fill(p, ('_'):byte())
    t:expect(t.expr(mem).read(p)):eq('__________')
    mem.write(p, 'abc', 5)
    t:expect(t.expr(mem).read(p)):eq('_____abc__')
end
