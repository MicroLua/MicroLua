-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local math = require 'math'
local int64 = require 'mlua.int64'
local list = require 'mlua.list'
local thread = require 'mlua.thread'
local time = require 'pico.time'
local table = require 'table'

local function for_each_time(t, fn)
    local done<close> = function() t:context() end
    for _, tfn in ipairs{'get_absolute_time', 'get_absolute_time_int'} do
        t:context{get_time = tfn}
        fn(time[tfn], tfn)
    end
end

function test_time_computation(t)
    for _, test in ipairs{
        {'to_ms_since_boot', {int64(12345678)}, 12345},
        {'delayed_by_us', {int64(1000000), 12345}, int64(1012345)},
        {'delayed_by_ms', {int64(1000000), 123}, int64(1123000)},
        {'absolute_time_diff_us', {int64(13579), int64(24680)}, 24680 - 13579},
        {'absolute_time_diff_us', {int64('0x123456789'), int64('0xabcdef012')},
         int64('0xabcdef012') - int64('0x123456789')},
        {'absolute_time_min', {int64(13579), int64(24680)}, int64(13579)},
        {'absolute_time_min', {int64(24680), int64(13579)}, int64(13579)},
        {'is_at_the_end_of_time', {int64(12345678)}, false},
        {'is_at_the_end_of_time', {time.at_the_end_of_time}, true},
        {'is_nil_time', {int64(12345678)}, false},
        {'is_nil_time', {time.nil_time}, true},
    } do
        local fn, args, want = table.unpack(test)
        t:expect(t.expr(time)[fn](table.unpack(args))):eq(want)
    end
end

function test_timeouts(t)
    for _, test in ipairs{
        {'make_timeout_time_us', 10000, 10000},
        {'make_timeout_time_us', int64('0x123456789'), int64('0x123456789')},
        {'make_timeout_time_ms', 10, 10000},
        {'make_timeout_time_ms', 2147483647, int64('2147483647000')},
    } do
        local fn, arg, want = table.unpack(test)
        local now = want <= math.maxinteger and time.get_absolute_time_int()
                    or time.get_absolute_time()
        local got = time[fn](arg) - now
        t:expect(got):label('timeout'):gte(want):lt(want + 200)
    end
end

function test_sleep_BNB(t)
    local delay = 2000
    for_each_time(t, function(get_time)
        local t1 = get_time()
        time.sleep_until(t1 + delay)
        local t2 = get_time()
        t:expect(t2 - t1):label("sleep_until(%s) duration", t1 + delay)
            :gte(delay):lt(delay + 250)
    end)

    local t1 = time.get_absolute_time_int()
    time.sleep_us(delay)
    local t2 = time.get_absolute_time_int()
    t:expect(t2 - t1):label("sleep_us(%s) duration", delay)
        :gte(delay):lt(delay + 250)

    local t1 = time.get_absolute_time_int()
    time.sleep_ms(delay // 1000)
    local t2 = time.get_absolute_time_int()
    t:expect(t2 - t1):label("sleep_ms(%s) duration", delay)
        :gte(delay):lt(delay + 250)
end

function test_best_effort_wfe_or_timeout(t)
    for_each_time(t, function(get_time)
        local start = get_time()
        for i = 0, 2 do
            local want = i * 4000
            local reached = time.best_effort_wfe_or_timeout(start + want)
            local got = get_time() - start
            local ex = t:expect(got):label('time'):lt(want + 300)
            if reached then ex:gte(want) end
        end
    end)
end

function test_add_alarm(t)
    -- Test absolute alarms.
    for_each_time(t, function(get_time)
        local start = get_time()
        for _, test in ipairs{
            {start + 1000, true, start + 1000},
            {start - 1000, false, nil},
            {start - 1000, true, 0},
        } do
            local arg, fip, want = table.unpack(test)
            local t1, t2, alarm = get_time()
            alarm = time.add_alarm_at(arg, function(a)
                t2 = get_time()
                t:expect(a):label("alarm"):eq(alarm)
            end, fip)
            if alarm then
                alarm:join()
                if want == 0 then want = t1 end
                t:expect(t2):label("add_alarm_at(%s): end time", arg)
                    :gte(want):lt(want + 800)
            end
            t:expect(alarm ~= nil):label("alarm set"):eq(want ~= nil)
        end
    end)

    -- Test relative alarms.
    for _, test in ipairs{
        {'add_alarm_in_us', 1000, true, 1000},
        {'add_alarm_in_ms', 1, true, 1000},
    } do
        local fn, arg, fip, want = table.unpack(test)
        local t1, t2, alarm = time.get_absolute_time_int()
        alarm = time[fn](arg, function(a)
            t2 = time.get_absolute_time_int()
            t:expect(a):label("alarm"):eq(alarm)
        end, fip)
        if alarm then
            alarm:join()
            want = t1 + want
            t:expect(t2):label("%s(%s): end time", fn, arg)
                :gte(want):lt(want + 800)
        end
        t:expect(alarm ~= nil):label("alarm set"):eq(want ~= nil)
    end
end

function test_alarm_repeat(t)
    for_each_time(t, function(get_time)
        local res, got = {-2000, 2000, 0}, list()
        local start = get_time()
        local alarm = time.add_alarm_at(start + 1000, function()
            got:append(get_time())
            time.sleep_us(1000)
            return res[#got]
        end)
        alarm:join()

        local want = {1000, 3000, 6000}
        t:expect(t.expr(got):len()):eq(#want)
        for i = 1, #want do
            t:expect(got[i] - start):label("alarm[%s]", i)
                :gte(want[i]):lt(want[i] + 1500)
        end
    end)
end

function test_cancel_alarm(t)
    local parent = thread.running()

    -- Cancel before the alarm thread starts.
    local triggered = false
    local alarm = time.add_alarm_in_ms(10000, function() triggered = true end)
    t:expect(t.expr(time).cancel_alarm(alarm)):eq(true)
    t:expect(triggered):label("triggered"):eq(false)

    -- Cancel before the sleep elapses.
    local triggered = false
    local alarm = time.add_alarm_in_ms(10000, function() triggered = true end)
    thread.yield()
    t:expect(t.expr(time).cancel_alarm(alarm)):eq(true)
    t:expect(triggered):label("triggered"):eq(false)

    -- Cancel while the callback runs.
    local triggered = false
    local alarm = time.add_alarm_in_us(800, function()
        parent:resume()
        thread.yield()
        triggered = true
    end)
    thread.suspend()
    t:expect(t.expr(time).cancel_alarm(alarm)):eq(true)
    t:expect(triggered):label("triggered"):eq(false)

    -- Cancel after the callback terminates.
    local triggered = false
    local alarm = time.add_alarm_in_us(100, function() triggered = true end)
    alarm:join()
    t:expect(t.expr(time).cancel_alarm(alarm)):eq(false)
    t:expect(triggered):label("triggered"):eq(true)
end

function test_add_repeating_timer(t)
    for_each_time(t, function(get_time)
        for _, test in ipairs{
            {'add_repeating_timer_us', 1000},
            {'add_repeating_timer_ms', 1},
        } do
            local fn, fact = table.unpack(test)
            local res, got = list.pack(nil, -3000, true, 1000, false), list()
            local start = get_time()
            local alarm = time[fn](-2 * fact, function()
                got:append(get_time())
                time.sleep_us(1000)
                return res[#got]
            end)
            alarm:join()

            local want = {2000, 4000, 7000, 10000, 12000}
            t:expect(t.expr(got):len()):eq(#want)
            for i = 1, #want do
                t:expect(got[i] - start):label("%s: alarm[%s]", fn, i)
                    :gte(want[i]):lt(want[i] + 1500)
            end
        end
    end)
end
