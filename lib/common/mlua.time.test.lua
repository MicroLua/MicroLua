-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local math = require 'math'
local time = require 'mlua.time'
local table = require 'table'

function test_ticks(t)
    t:expect(t:expr(time).ticks_per_second):eq(1e6)
    t:expect(t:expr(time).ticks64())
        :gte(time.min_ticks):lte(time.max_ticks)
    for i = 1, 10 do
        local t1 = time.ticks64()
        local t2 = time.ticks64()
        while t2 == t1 do t2 = time.ticks64() end
        t:expect(t2 - t1):label("t2 - t1"):gt(0):lt(150)
    end

    t:expect(t:expr(time).ticks()):apply(math.type):op('type is'):eq('integer')
    for i = 1, 10 do
        local t1 = time.ticks()
        local t2 = time.ticks()
        while t2 == t1 do t2 = time.ticks() end
        t:expect(t2 - t1):label("t2 - t1"):gt(0):lt(100)
    end
end

function test_sleep_BNB(t)
    local start = time.ticks64()
    for _, test in ipairs{
        {'sleep_until', start + 3000, start + 3000, 0},
        {'sleep_for', 4000, 0, 4000},
    } do
        local fn, arg, want, want_delta = table.unpack(test)
        local t1 = time.ticks64()
        time[fn](arg)
        local t2 = time.ticks64()
        if want == 0 then want = t1 + want_delta end
        t:expect(t2):label("%s(%s): end time", fn, arg)
            :gte(want):lt(want + 500)
    end
end
