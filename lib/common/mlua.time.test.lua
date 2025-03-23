-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local math = require 'math'
local int64 = require 'mlua.int64'
local time = require 'mlua.time'
local string = require 'string'
local table = require 'table'

local function for_each_ticks(t, fn)
    local done<close> = function() t:context() end
    for _, tfn in ipairs{'ticks', 'ticks64'} do
        t:context{ticks = tfn}
        fn(time[tfn], tfn)
    end
end

function test_ticks(t)
    t:expect(t.expr(time).usec):eq(1)
    t:expect(t.expr(time).msec):eq(1000)
    t:expect(t.expr(time).sec):eq(1000 * 1000)
    t:expect(t.expr(time).min):eq(60 * 1000 * 1000)
    t:expect(t.expr(time).ticks64())
        :gte(time.min_ticks):lte(time.max_ticks)
    t:expect(t.expr(time).ticks()):apply(math.type):op('type is'):eq('integer')

    for_each_ticks(t, function(ticks)
        for i = 1, 10 do
            local t1 = ticks()
            local t2 = ticks()
            while t2 == t1 do t2 = ticks() end
            t:expect(t2 - t1):label("t2 - t1"):gt(0):lt(100)
        end
    end)
end

local integer_bits = string.packsize('j') * 8

function test_to_ticks64(t)
    for _, test in ipairs{
        {0, int64('0'), int64('0')},
        {0x12345678, int64('0x1a2b3c4d11111111'), int64('0x1a2b3c4d12345678')},
        {0x7fffffff, int64('0x1a2b3c4d00000000'), int64('0x1a2b3c4d7fffffff')},
        {0x80000000, int64('0x1a2b3c4d00000000'), int64('0x1a2b3c4c80000000')},
        {0x7ffffffe, int64('0x1a2b3c4dffffffff'), int64('0x1a2b3c4e7ffffffe')},
        {0x7fffffff, int64('0x1a2b3c4dffffffff'), int64('0x1a2b3c4d7fffffff')},
    } do
        local ti, now, want = table.unpack(test)
        if integer_bits < 64 then
            t:expect(t.expr(time).to_ticks64(ti, now)):eq(want)
            t:expect(time.to_ticks64(ti, now) - now)
                :label("tick64(%s, now) - now", ti)
                :gte(math.mininteger):lte(math.maxinteger)
        else
            t:expect(t.expr(time).to_ticks64(ti, now)):eq(ti)
        end
    end
end

function test_compare(t)
    local now = time.ticks()
    for _, test in ipairs{
        {now, now, 0},
        {now - 1, now, -1},
        {now + 1, now, 1},
        {int64('0x0123456789abcdef'), int64('0x0123456789abcdef'), 0},
        {int64('0x0123456789abcdee'), int64('0x0123456789abcdef'), -1},
        {int64('0x0123456789abcdf0'), int64('0x0123456789abcdef'), 1},
        {time.min_ticks, time.max_ticks, -1},
        {time.max_ticks, time.min_ticks, 1},
    } do
        local lhs, rhs, want = table.unpack(test)
        t:expect(t.expr(time).compare(lhs, rhs)):eq(want)
    end
end

function test_diff(t)
    local now = time.ticks()
    for _, test in ipairs{
        {now, now, 0},
        {now - 1, now + 2, 3},
        {now + 3, now - 4, -7},
        {int64('0x0123456789abcdef'), int64('0x7123456789abcdef'),
         int64('0x7000000000000000')},
        {int64('0x7123456789abcdef'), int64('0x0123456789abcdef'),
         int64('-0x7000000000000000')},
        {time.min_ticks, time.max_ticks, time.max_ticks - time.min_ticks},
        {time.max_ticks, time.min_ticks, time.min_ticks - time.max_ticks},
    } do
        local lhs, rhs, want = table.unpack(test)
        t:expect(t.expr(time).diff(lhs, rhs)):eq(want)
    end
end

function test_deadline(t)
    for _, test in ipairs{10000, int64('0x123456789')} do
        local now = test <= math.maxinteger and time.ticks() or time.ticks64()
        local got = time.deadline(test) - now
        t:expect(got):label('timeout'):gte(test):lt(test + 200)
    end
end

function test_sleep_BNB(t)
    local delay = 2000
    for_each_ticks(t, function(ticks)
        local t1 = ticks()
        time.sleep_until(t1 + delay)
        local t2 = ticks()
        t:expect(t2 - t1):label("sleep_until(%s) duration", t1 + delay)
            :gte(delay):lt(delay + 250)
    end)

    local t1 = time.ticks()
    time.sleep_for(delay)
    local t2 = time.ticks()
    t:expect(t2 - t1):label("sleep_for(%s) duration", delay)
        :gte(delay):lt(delay + 200)
end
