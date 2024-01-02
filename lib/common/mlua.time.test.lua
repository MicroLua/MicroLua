-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local time = require 'mlua.time'

function test_ticks(t)
    t:expect(t:expr(time).ticks_per_second):eq(1e6)
    t:expect(t:expr(time).ticks())
        :gte(time.min_ticks):lte(time.max_ticks)

    local prev = time.min_ticks
    for i = 1, 10 do
        local cur = time.ticks()
        t:expect(cur):label("ticks()"):gt(prev)
        prev = cur
    end
end
