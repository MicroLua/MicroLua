_ENV = mlua.Module(...)

local int64 = require 'mlua.int64'
local thread = require 'mlua.thread'
local time = require 'pico.time'
local table = require 'table'

function test_time_computation(t)
    for _, test in ipairs{
        {'to_ms_since_boot', {12345678}, 12345},
        {'delayed_by_us', {1000000, 12345}, int64(1012345)},
        {'delayed_by_ms', {1000000, 123}, int64(1123000)},
        {'absolute_time_diff_us', {13579, 24680}, int64(24680 - 13579)},
        {'absolute_time_min', {13579, 24680}, int64(13579)},
        {'absolute_time_min', {24680, 13579}, int64(13579)},
        {'is_at_the_end_of_time', {12345678}, false},
        {'is_at_the_end_of_time', {time.at_the_end_of_time}, true},
        {'is_nil_time', {12345678}, false},
        {'is_nil_time', {time.nil_time}, true},
    } do
        local fn, args, want = table.unpack(test)
        t:expect(t:expr(time)[fn](table.unpack(args))):eq(want)
    end
end

function test_timeouts(t)
    for _, test in ipairs{
        {'make_timeout_time_us', 10000, 10000},
        {'make_timeout_time_ms', 10, 10000},
    } do
        local fn, arg, want = table.unpack(test)
        local now = time.get_absolute_time()
        local got = time[fn](arg) - now
        t:expect(got):label('timeout'):gte(want):lt(want + 200)
    end
end

function test_sleep(t)
    local start = time.get_absolute_time()
    for _, test in ipairs{
        {'sleep_until', start + 3000, start + 3000, 0},
        {'sleep_us', 4000, 0, 4000},
        {'sleep_ms', 6, 0, 6000},
    } do
        local fn, arg, want, want_delta = table.unpack(test)
        local t1 = time.get_absolute_time()
        time[fn](arg)
        local t2 = time.get_absolute_time()
        if want == 0 then want = t1 + want_delta end
        t:expect(t2 >= want, "%s(%s) waited from %s to %s, want >= %s", fn, arg,
                 t1, t2, want)
    end
end

function test_best_effort_wfe_or_timeout(t)
    local start = time.get_absolute_time()
    for i = 0, 4 do
        local want = start + i * 3000
        local reached = time.best_effort_wfe_or_timeout(want)
        local got = time.get_absolute_time()
        local ex = t:expect(got):label('time'):lt(want + 200)
        if reached then ex:gte(want) end
    end
end

function test_alarms(t)
    if not time.add_alarm_at then t:skip("Default alarm pool disabled") end
    local start = time.get_absolute_time()
    for _, test in ipairs{
        {'add_alarm_at', start + 3000, start + 3000, 0},
        {'add_alarm_in_us', 4000, 0, 4000},
        {'add_alarm_in_ms', 6, 0, 6000},
    } do
        local fn, arg, want, want_delta = table.unpack(test)
        local t1, t2, alarm = time.get_absolute_time(), nil, nil
        alarm = time[fn](arg, function(a)
            t2 = time.get_absolute_time()
            t:expect(a):label("alarm"):eq(alarm)
        end)
        alarm:join()
        if want == 0 then want = t1 + want_delta end
        t:expect(t2):label("%s(%s): end time", fn, arg):gte(want)
    end

    -- TODO: Test cancel_alarm
end
