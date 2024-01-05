-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local time = require 'mlua.time'
local table = require 'table'

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

function test_sleep_Y(t)
    local start = time.ticks()
    for _, test in ipairs{
        {'sleep_until', start + 3000, start + 3000, 0},
        {'sleep_for', 4000, 0, 4000},
    } do
        local fn, arg, want, want_delta = table.unpack(test)
        local t1 = time.ticks()
        time[fn](arg)
        local t2 = time.ticks()
        if want == 0 then want = t1 + want_delta end
        t:expect(t2):label("%s(%s): end time", fn, arg)
            :gte(want):lt(want + 500)
    end
end
