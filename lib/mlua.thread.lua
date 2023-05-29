-- A cooperative threading library.

_ENV = require 'mlua.module'(...)

local coroutine = require 'coroutine'
local signal = require 'mlua.signal'
local time = require 'pico.time'
local string = require 'string'
local table = require 'table'

local active = {}
local head, tail = 0, 0
local waiting = {}
local timers = {}

-- Add a waiting task to the timer list.
local function add_timer(task, deadline)
    local l, r = 1, #timers + 1
    while l < r do
        local m = (l + r) // 2
        if waiting[timers[m]] > deadline then r = m else l = m + 1 end
    end
    table.insert(timers, r, task)
end

-- Remove a waiting task from the timer list.
local function remove_timer(task)
    local deadline = waiting[task]
    local l, r = 1, #timers + 1
    while l < r do
        local m = (l + r) // 2
        local t = timers[m]
        if t == task then
            table.remove(timers, l)
            return
        end
        if waiting[t] < deadline then l = m + 1 else r = m end
    end
    for i = l, #timers do
        if timers[i] == task then
            table.remove(timers, i)
            return
        end
    end
end

-- Remove the first cnt entries from the timer list.
local function remove_timers(cnt)
    local len = #timers
    table.move(timers, cnt + 1, len, 1)
    for i = len - cnt + 1, len do
        timers[i] = nil
    end
end

-- Start a new task calling the given function.
function start(fn)
    local task = coroutine.create(fn)
    active[tail] = task
    tail = tail + 1
    signal.wake()
    return task
end

-- Return the currently-running task.
running = coroutine.running

-- Make the currently-running task yield. If the argument is true, the task is
-- moved from the active queue to the wait list. If the argument is an absolute
-- time, the task is resumed at that time (unless it's resumed explicitly
-- earlier).
yield = coroutine.yield

-- Return the current time.
now = time.get_absolute_time

-- Move the given task from the wait list to the active queue.
function resume(task)
    local deadline = waiting[task]
    if deadline then
        if deadline ~= true then remove_timer(task) end
        waiting[task] = nil
        active[tail] = task
        tail = tail + 1
        signal.wake()
    end
end

-- Resume the tasks whose deadline has elapsed.
local function resume_deadlined()
    local len = #timers
    if len == 0 then return end
    local i, t = 1, now()
    while i <= len do
        local task = timers[i]
        if waiting[task] > t then break end
        waiting[task] = nil
        active[tail] = task
        tail = tail + 1
        i = i + 1
    end
    if i > 1 then remove_timers(i - 1) end
end

-- Run the task dispatch loop.
function main(fn)
    if fn then start(fn) end
    while true do
        -- Dispatch signals and wait for at least one active task.
        signal.dispatch(head ~= tail and time.nil_time or waiting[timers[1]]
                        or time.at_the_end_of_time)

        -- Resume tasks whose deadline has elapsed.
        resume_deadlined()

        -- Resume the task at the head of the active queue.
        local task = active[head]
        active[head] = nil
        head = head + 1
        local _, deadline = coroutine.resume(task)

        -- Close the task if it's dead, park it if it is suspended, or move it
        -- to the end of the active queue.
        if coroutine.status(task) == 'dead' then
            local ok, err = coroutine.close(task)
            if not ok then
                stderr:write(string.format("Task failed: %s\n", err))
            end
        elseif deadline then
            waiting[task] = deadline
            if deadline ~= true then add_timer(task, deadline) end;
        else
            active[tail] = task
            tail = tail + 1
        end
    end
end
