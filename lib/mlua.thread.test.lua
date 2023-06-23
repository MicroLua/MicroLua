_ENV = require 'mlua.module'(...)

local thread = require 'mlua.thread'
local string = require 'string'

function test_Thread_name(t)
    local got, want = thread.running():name(), 'main'
    t:expect(got == want,
             "Unexpected main thread name: got %q, want %q", got, want)
    local th = thread.start(function() end)
    got, want = th:name(), tostring(th):gsub('^[^:]+: ([0-9A-F]+)$', '%1')
    t:expect(got == want,
             "Unexpected default thread name: got %q, want %q", got, want)
    want = 'some-thread'
    th = thread.start(function() end):set_name(want)
    got = th:name()
    t:expect(got == want,
             "Unexpected custom thread name: got %q, want %q", got, want)
end

function test_Thread_suspend_resume(t)
    local started = false
    local th
    th = thread.start(function()
        local got = thread.running()
        t:expect(got == th,
                 "Unexpected running thread: got %s, want %s", got, th)
        thread.yield(true)
    end)
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

function test_Thread_join(t)
    local th = thread.start(function() thread.yield() end)
    thread.yield()
    t:expect(th:is_alive(), "Thread isn't alive")
    th:join()
    t:expect(not th:is_alive(), "Thread is alive")

    do
        local closing<close> = thread.start(function() thread.yield() end)
        th = closing
        t:expect(th:is_alive(), "Thread isn't alive")
    end
    t:expect(not th:is_alive(), "Thread is alive")

    local th = thread.start(function() error("boom", 0) end)
    local ok, err = pcall(function() th:join() end)
    t:assert(not ok, "join didn't throw an error")
    t:expect(err == 'boom', "Unexpected error: got %q, want \"boom\"", err)
end

function test_yield(t)
    local log = ''
    local done = 0
    for t = 1, 5 do
        thread.start(function()
            for i = 1, 3 do
                log = log .. ('(%s, %s) '):format(t, i)
                thread.yield()
            end
            done = done + 1
        end)
    end
    while done < 5 do thread.yield() end
    t:expect(log == '(1, 1) (2, 1) (3, 1) (4, 1) (5, 1) '
                 .. '(1, 2) (2, 2) (3, 2) (4, 2) (5, 2) '
                 .. '(1, 3) (2, 3) (3, 3) (4, 3) (5, 3) ',
             "Unexpected execution sequence: %s", log)
end
