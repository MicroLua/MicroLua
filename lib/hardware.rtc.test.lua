_ENV = mlua.Module(...)

local rtc = require 'hardware.rtc'
local thread = require 'mlua.thread'
local util = require 'mlua.util'

-- The RTC is started and set in 00_setup.test.

function test_get_datetime(t)
    t:expect(t:expr(rtc).running()):eq(true)
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
    thread.yield(true)
    t:expect(triggered):label("triggered"):eq(true)
end
