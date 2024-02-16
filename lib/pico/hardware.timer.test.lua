-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local timer = require 'hardware.timer'
local math = require 'math'
local int64 = require 'mlua.int64'
local thread = require 'mlua.thread'
local table = require 'table'

function test_time_us(t)
    local ti = timer.time_us()
    local t32 = timer.time_us_32()
    local t64 = timer.time_us_64()
    t32 = int64(t32) + (t32 < 0 and (int64(1) << 32) or 0)
    t64 = t64 & ((int64(1) << 32) - 1)
    t:expect(t64 - t32):label("t64 - t32"):gte(0):lt(100)
    if math.maxinteger <= 0x7fffffff then
        t:expect(t32 - ti):label("t32 - ti"):gte(0):lt(100)
    else
        t:expect(t64 - ti):label("t64 - ti"):gte(0):lt(100)
    end
end

function test_time_reached(t)
    for _, fn in ipairs{'time_us', 'time_us_64'} do
        local time_us = timer[fn]
        local now = time_us()
        t:expect(timer.time_reached(now))
            :label("time_reached(%s())", fn):eq(true)
        local now = time_us()
        t:expect(timer.time_reached(now + 150))
            :label("time_reached(%s() + 150)", fn):eq(false)
    end
end

function test_busy_wait(t)
    for _, tfn in ipairs{'time_us', 'time_us_64'} do
        local time_us = timer[tfn]
        for _, test in ipairs{
            {'busy_wait_until', 1, 3000, 3000},
            {'busy_wait_us', 0, 4000, 4000},
            {'busy_wait_us_32', nil, 5000, 5000},
            {'busy_wait_ms', nil, 6, 6000},
        } do
            local fn, orig, delta, delay = table.unpack(test)
            local t1 = time_us()
            timer[fn]((orig and orig * t1 or 0) + delta)
            local t2 = time_us()
            t:expect(t2 - t1):label("%s(%s) duration (%s)", fn, delta, tfn)
                :gte(delay):lt(delay + 250)
        end
    end
end

function test_timer(t)
    local alarm = timer.claim_unused()
    t:cleanup(function()
        timer.unclaim(alarm)
        t:expect(not timer.is_claimed(alarm),
                 "Unclaimed alarm reported as claimed")
    end)
    t:expect(timer.is_claimed(alarm), "Claimed alarm reported as unclaimed")

    -- Set up the alarm callback.
    local parent, got, time_us = thread.running()
    timer.set_callback(alarm, function(a)
        got = time_us()
        t:expect(a):label("alarm"):eq(alarm)
        parent:resume()
    end)
    t:cleanup(function() timer.set_callback(alarm, nil) end)

    -- Trigger alarms.
    for _, fn in ipairs{'time_us', 'time_us_64'} do
        time_us = timer[fn]
        local start = time_us()
        t:expect(timer.set_target(alarm, start),
                 "Setting missed alarm succeeded (%s)", fn)
        for delta = 10000, 40000, 3000 do
            local err = timer.set_target(alarm, start + delta)
            t:assert(not err, "Failed to set alarm at +%s us (%s)", delta, fn)
            thread.suspend()
            t:expect(got - start):label("alarm time (%s)", fn)
                :gte(delta):lt(delta + 200)
        end
    end
end

function test_irq_scheduling_latency(t)
    local samples = 100
    local alarm = timer.claim_unused()
    t:cleanup(function() timer.unclaim(alarm) end)
    local time_us_32 = timer.time_us_32
    local parent, want, count, min, max, sum = thread.running()
    timer.set_callback(alarm, function()
        local got = time_us_32()
        local delta = got - int64.tointeger(want & 0xffffffff)
        if delta < min then min = delta end
        if delta > max then max = delta end
        sum = sum + delta
        count = count + 1
        if count >= samples then
            parent:resume()
            return
        end
        want = want + 500
        timer.set_target(alarm, want)
    end)
    t:cleanup(function() timer.set_callback(alarm, nil) end)

    for _, fn in ipairs{'time_us', 'time_us_64'} do
        local time_us = timer[fn]
        count, min, max, sum = 0, math.maxinteger, math.mininteger, 0
        want = time_us() + 1000
        timer.set_target(alarm, want)
        thread.suspend()
        t:printf("Samples: %2s, min: %2s us, max: %2s us, avg: %2.1f us (%s)\n",
                 count, min, max, sum / count, fn)
    end
end
