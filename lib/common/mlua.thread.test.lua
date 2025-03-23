-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local coroutine = require 'coroutine'
local math = require 'math'
local int64 = require 'mlua.int64'
local thread = require 'mlua.thread'
local group = require 'mlua.thread.group'
local time = require 'mlua.time'
local string = require 'string'

function test_Thread_name(t)
    t:expect(t.expr(thread).running():name()):eq('main')
    local th1<close> = thread.start(function() end)
    t:expect(t.expr.th1:name())
        :eq((tostring(th1):gsub('^[^:]+: (0?x?[0-9a-fA-F]+)$', '%1')))
    local want = 'some-thread'
    local th2<close> = thread.start(function() end, want)
    t:expect(t.expr.th2:name()):eq(want)
end

function test_Thread_suspend_resume(t)
    local want
    local th<close> = thread.start(function()
        t:expect(t.expr(thread).running()):eq(want)
        thread.suspend()
    end)
    want = th
    t:expect(not th:is_waiting(), "Started thread is waiting")
    t:expect(th:is_alive(), "Started thread isn't alive")
    thread.yield()
    t:expect(th:is_waiting(), "Thread hasn't suspended")
    t:expect(th:is_alive(), "Suspended thread isn't alive")
    th:resume()
    t:expect(not th:is_waiting(), "Resumed thread is waiting")
    thread.yield()
    t:expect(not th:is_waiting(), "Terminated thread is waiting")
    t:expect(not th:is_alive(), "Terminated thread is alive")
end

function test_Thread_kill(t)
    t:expect(t.expr(thread).running():kill()):raises("kill itself")
    for _, suspend in ipairs{false, true} do
        local closed = false
        local th<close> = thread.start(function()
            local done<close> = function() closed = true end
            if suspend then thread.suspend() else thread.yield() end
            error("never reached")
        end)
        thread.yield()
        t:expect(t.expr(th):is_alive()):eq(true)
        t:expect(th:is_alive(), "Thread is dead before kill")
        t:expect(not closed, "Cleanup has run before kill")
        t:expect(t.expr(th):kill()):eq(true)
        t:expect(not th:is_alive(), "Killed thread is alive")
        t:expect(closed, "Cleanup hasn't run on kill")
        t:expect(t.expr(th):kill()):eq(false)
    end
end

function test_Thread_join(t)
    local th1<close> = thread.start(function() thread.yield() end)
    thread.yield()
    t:expect(th1:is_alive(), "Thread isn't alive")
    th1:join()
    t:expect(not th1:is_alive(), "Joined thread is alive")

    local th2
    do
        local closing<close> = thread.start(function() thread.yield() end)
        th2 = closing
        t:expect(th2:is_alive(), "Thread isn't alive")
    end
    t:expect(not th2:is_alive(), "Joined thread is alive")

    local th3 = thread.start(function() error("boom", 0) end)
    local ok, err = pcall(function() th3:join() end)
    t:assert(not ok, "join didn't raise an error")
    t:expect(err):label('error'):eq("boom")
end

function test_active(t)
    local log = ''
    local ths<close> = thread.Group()
    for t = 1, 5 do
        ths:start(function()
            for i = 1, 3 do
                log = log .. ('(%s, %s) '):format(t, i)
                thread.yield()
            end
        end)
    end
    ths:join()
    t:expect(log):label("log"):eq('(1, 1) (2, 1) (3, 1) (4, 1) (5, 1) ' ..
                                  '(1, 2) (2, 2) (3, 2) (4, 2) (5, 2) ' ..
                                  '(1, 3) (2, 3) (3, 3) (4, 3) (5, 3) ')
end

function test_active_spinning(t)
    local i = 0
    local th<close> = thread.start(function()
        while i < 1000 do
            i = i + 1
            thread.yield()
        end
    end)
    th:join()
    t:expect(i):label("i"):eq(1000)
end

function test_timers(t)
    local log = ''
    local ths<close> = thread.Group()
    local start = time.ticks()
    for t = 1, 5 do
        ths:start(function()
            for i = 1, 3 do
                log = log .. ('(%s, %s) '):format(t, i)
                thread.suspend(start + (i * 5000 + ((t + 2) % 5) * 500))
            end
        end)
    end
    ths:join()
    t:expect(log):label("log"):eq('(1, 1) (2, 1) (3, 1) (4, 1) (5, 1) ' ..
                                  '(3, 2) (4, 2) (5, 2) (1, 2) (2, 2) ' ..
                                  '(3, 3) (4, 3) (5, 3) (1, 3) (2, 3) ')
end

function test_active_and_timers(t)
    local log = ''
    local ths<close> = thread.Group()
    ths:start(function()
        log = log .. 'a'
        thread.yield()
        log = log .. 'b'
        thread.yield()
        log = log .. 'c'
    end)
    ths:start(function()
        log = log .. 'd'
        thread.suspend(time.min_ticks + 1)
        log = log .. 'e'
        thread.suspend(time.min_ticks + 3)
        log = log .. 'f'
    end)
    ths:start(function()
        log = log .. 'g'
        thread.suspend(time.min_ticks + 2)
        log = log .. 'h'
        thread.suspend(time.min_ticks + 4)
        log = log .. 'i'
    end)
    ths:join()
    t:expect(log):label("log"):eq('adgbehcfi')
end

function test_blocking(t)
    t:cleanup(function() thread.blocking(false) end)
    local ths<close> = thread.Group()
    t:expect(t.expr(thread).blocking()):eq(false)
    ths:start(function()
        t:expect(t.expr(thread).blocking()):eq(false)
    end)
    t:expect(t.expr(thread).blocking(true)):eq(false)
    t:expect(t.expr(thread).blocking()):eq(true)
    ths:start(function()
        t:expect(t.expr(thread).blocking()):eq(true)
    end)
    t:expect(t.expr(thread).blocking(false)):eq(true)
    t:expect(t.expr(thread).blocking()):eq(false)
    ths:start(function()
        t:expect(t.expr(thread).blocking()):eq(false)
    end)
    ths:join()
end

function test_scheduling_latency(t)
    local samples = 10
    local ticks, sleep_until = time.ticks, time.sleep_until
    for _, count in ipairs{1, 2, 4, 8, 16} do
        local min, max, sum = math.maxinteger, math.mininteger, 0
        local threads = thread.Group()
        for i = 1, count do
            threads:start(function()
                for j = 1, samples do
                    local want = ticks() + 5000
                    sleep_until(want)
                    local got = ticks()
                    local delta = got - want
                    if delta < min then min = delta end
                    if delta > max then max = delta end
                    sum = sum + delta
                end
            end)
            time.sleep_for(20)
        end
        threads:join()
        t:printf("Threads: %2s, min: %2s us, max: %2s us, avg: %4.1f us\n",
                 count, min, max, sum / (count * samples))
        collectgarbage()
    end
end
