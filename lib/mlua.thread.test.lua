-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local thread = require 'mlua.thread'
local string = require 'string'

function test_Thread_name(t)
    t:expect(t:expr(thread).running():name()):eq('main')
    local th1<close> = thread.start(function() end)
    t:expect(t.expr.th1:name())
        :eq((tostring(th1):gsub('^[^:]+: ([0-9A-F]+)$', '%1')))
    local want = 'some-thread'
    local th2<close> = thread.start(function() end):set_name(want)
    t:expect(t.expr.th2:name()):eq(want)
end

function test_Thread_suspend_resume(t)
    local want
    local th<close> = thread.start(function()
        t:expect(t:expr(thread).running()):eq(want)
        thread.yield(true)
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
    t:expect(t:expr(thread).running():kill()):raises("running coroutine")
    for _, suspend in ipairs{false, true} do
        local closed = false
        local th<close> = thread.start(function()
            local done<close> = function() closed = true end
            thread.yield(suspend)
            error("never reached")
        end)
        thread.yield()
        t:expect(t:expr(th):is_alive()):eq(true)
        t:expect(th:is_alive(), "Thread is dead before kill")
        t:expect(not closed, "Cleanup has run before kill")
        th:kill()
        t:expect(not th:is_alive(), "Killed thread is alive")
        t:expect(closed, "Cleanup hasn't run on kill")
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

function test_yield(t)
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
    ths.join(ths)
    t:expect(log == '(1, 1) (2, 1) (3, 1) (4, 1) (5, 1) '
                 .. '(1, 2) (2, 2) (3, 2) (4, 2) (5, 2) '
                 .. '(1, 3) (2, 3) (3, 3) (4, 3) (5, 3) ',
             "Unexpected execution sequence: %s", log)
end
