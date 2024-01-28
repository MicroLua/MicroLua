-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local timer = require 'hardware.timer'
local int64 = require 'mlua.int64'
local thread = require 'mlua.thread'
local table = require 'table'

function test_time_us(t)
    local t32, t64 = timer.time_us_32(), timer.time_us_64()
    t32 = int64(t32) + (t32 < 0 and (int64(1) << 32) or 0)
    t64 = t64 & ((int64(1) << 32) - 1)
    t:expect(t64 - t32):label("t64 - t32"):gte(0):lt(100)
end

function test_time_reached(t)
    local now = timer.time_us_64()
    t:expect(timer.time_reached(now), "Current time not reached: %s", now)
    local now = timer.time_us_64()
    t:expect(not timer.time_reached(now + 100), "Reached now + 100")
end

function test_busy_wait(t)
    local start = timer.time_us_64()
    for _, test in ipairs{
        {'busy_wait_until', start + 3000, start + 3000, 0},
        {'busy_wait_us', 4000, 0, 4000},
        {'busy_wait_us_32', 5000, 0, 5000},
        {'busy_wait_ms', 6, 0, 6000},
    } do
        local fn, arg, want, want_delta = table.unpack(test)
        local t1 = timer.time_us_64()
        timer[fn](arg)
        local t2 = timer.time_us_64()
        if want == 0 then want = t1 + want_delta end
        t:expect(t2 >= want, "%s(%s) waited from %s to %s, want >= %s", fn, arg,
                 t1, t2, want)
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
    local parent, got = thread.running(), nil
    timer.set_callback(alarm, function(a)
        got = timer.time_us_64()
        t:expect(a):label("alarm"):eq(alarm)
        parent:resume()
    end)
    t:cleanup(function() timer.set_callback(alarm, nil) end)

    -- Trigger alarms.
    local start = timer.time_us_64()
    t:expect(timer.set_target(alarm, start), "Setting missed alarm succeeded")
    for delta = 10000, 50000, 3000 do
        local err = timer.set_target(alarm, start + delta)
        t:assert(not err, "Failed to set alarm at +%s us", delta)
        thread.suspend()
        t:expect(got - start):label("alarm time"):gte(delta):lt(delta + 200)
    end
end
