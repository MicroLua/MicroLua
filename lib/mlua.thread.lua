-- A cooperative threading library.

_ENV = require 'mlua.module'(...)

local coroutine = require 'coroutine'
local event = require 'mlua.event'
local time = require 'pico.time'
local string = require 'string'
local table = require 'table'

local active = {}
local head, tail = 0, 0
local waiting = {}
local timers = {[0] = 0}
local names = {}
setmetatable(names, {__mode = 'k'})

-- Add a waiting thread to the timer list.
local function add_timer(thread, deadline)
    local len = timers[0]
    local l, r = 1, len + 1
    while l < r do
        local m = (l + r) // 2
        if waiting[timers[m]] > deadline then r = m else l = m + 1 end
    end
    table.insert(timers, r, thread)
    timers[0] = len + 1
end

-- Remove a waiting thread from the timer list.
local function remove_timer(thread)
    local len = timers[0]
    local deadline = waiting[thread]
    local l, r = 1, len + 1
    while l < r do
        local m = (l + r) // 2
        local t = timers[m]
        if t == thread then
            table.remove(timers, l)
            timers[0] = len - 1
            return
        end
        if waiting[t] < deadline then l = m + 1 else r = m end
    end
    for i = l, len do
        if timers[i] == thread then
            table.remove(timers, i)
            timers[0] = len - 1
            return
        end
    end
end

-- Start a new thread calling the given function.
function start(name, fn)
    if fn == nil then fn, name = name, nil end
    local thread = coroutine.create(fn)
    names[thread] = name
    active[tail] = thread
    tail = tail + 1
    return thread
end

-- Return the name of the given thread.
function name(thread)
    return names[thread] or string.sub(tostring(thread), 9)
end

-- Return the current absolute time.
now = time.get_absolute_time

-- Return the currently-running thread.
running = coroutine.running

-- Make the running thread yield. If the argument is true, the thread is moved
-- from the active queue to the wait list. If the argument is an absolute time,
-- the thread is resumed at that time (unless it's resumed explicitly earlier).
yield = coroutine.yield

-- Suspend the running thread until the given absolute time.
sleep_until = time.sleep_until

-- Suspend the running thread for the given duration (in microseconds).
sleep_for = time.sleep_us

-- Move the given thread from the wait list to the active queue.
function resume(thread)
    local deadline = waiting[thread]
    if not deadline then return false end
    if deadline ~= true then remove_timer(thread) end
    waiting[thread] = nil
    active[tail] = thread
    tail = tail + 1
    return true
end

-- Resume the threads whose deadline has elapsed.
local function resume_deadlined()
    local len = timers[0]
    if len == 0 then return end
    local i, t = 1, now()
    while i <= len do
        local thread = timers[i]
        if waiting[thread] > t then break end
        waiting[thread] = nil
        active[tail] = thread
        tail = tail + 1
        i = i + 1
    end
    if i > 1 then
        table.move(timers, i, len, 1)
        for i = len - i + 2, len do
            timers[i] = nil
        end
        timers[0] = len - i + 1
    end
end

-- Run the thread dispatch loop.
function main(fn, thread_name)
    if fn then start(thread_name, fn) end
    while true do
        -- Dispatch events and wait for at least one active thread.
        event.dispatch(resume,
                       head ~= tail and time.nil_time or waiting[timers[1]]
                       or time.at_the_end_of_time)

        -- Resume threads whose deadline has elapsed.
        resume_deadlined()

        -- Resume the thread at the head of the active queue.
        local thread = active[head]
        active[head] = nil
        head = head + 1
        local _, deadline = coroutine.resume(thread)

        -- Close the thread if it's dead, move it to the wait list if it is
        -- suspended, or move it to the end of the active queue.
        if coroutine.status(thread) == 'dead' then
            local ok, err = coroutine.close(thread)
            if not ok then
                stderr:write(string.format(
                    "Thread [%s] failed: %s\n", name(thread), err))
            end
        elseif deadline then
            waiting[thread] = deadline
            if deadline ~= true then add_timer(thread, deadline) end;
        else
            active[tail] = thread
            tail = tail + 1
        end
    end
end
