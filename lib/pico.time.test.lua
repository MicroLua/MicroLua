_ENV = mlua.Module(...)

local int64 = require 'mlua.int64'
local util = require 'mlua.util'
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
        t:expect(0 <= got, "Short timeout, by: %s us", -got)
        t:expect(got < want + 200, "Long timeout, delay: %s us", got - want)
    end
end

function test_sleep(t)
    -- TODO
end

function test_best_effort_wfe_or_timeout(t)
    -- TODO
end
