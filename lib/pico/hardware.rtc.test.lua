-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local clocks = require 'hardware.clocks'
local regs_resets = require 'hardware.regs.resets'
local resets = require 'hardware.resets'
local rtc = require 'hardware.rtc'
local thread = require 'mlua.thread'
local util = require 'mlua.util'

function set_up(t)
    local clk_rtc_hz = clocks.get_hz(clocks.clk_rtc)
    t:cleanup(function()
        resets.reset_block(regs_resets.RESET_RTC_BITS)
        resets.unreset_block_wait(regs_resets.RESET_RTC_BITS)
        clocks.set_reported_hz(clocks.clk_rtc, clk_rtc_hz)
    end)

    -- Start and set the RTC, faking a 1000x faster clk_rtc. This avoids having
    -- to wait for up to 1s for an alarm to trigger.
    clocks.set_reported_hz(clocks.clk_rtc, clk_rtc_hz // 1000)
    rtc.init()
    t:assert(rtc.set_datetime({year = 2020, month = 1, day = 2, dotw = 4,
                               hour = 3, min = 4, sec = 0}),
             "failed to set RTC time")
end

function test_get_datetime(t)
    t:expect(t.expr(rtc).running()):eq(true)
    local dt = rtc.get_datetime()
    dt.sec = nil  -- Second field could be anything
    t:expect(dt):label("datetime")
        :eq({year = 2020, month = 1, day = 2, dotw = 4, hour = 3, min = 4},
            util.table_eq)
end

function test_alarm(t)
    local dt = rtc.get_datetime()
    dt.sec = nil  -- Trigger on all seconds within the current minute.
    local parent, triggered = thread.running(), false
    rtc.set_alarm(dt, function()
        triggered = true
        parent:resume()
    end)
    t:cleanup(function() rtc.set_alarm({}, nil) end)
    thread.suspend()
    t:expect(triggered):label("triggered"):eq(true)
end
