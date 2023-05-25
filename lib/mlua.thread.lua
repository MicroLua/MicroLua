-- A cooperative threading library.

_ENV = require 'mlua.module'(...)

local coroutine = require 'coroutine'
local signal = require 'mlua.signal'
local string = require 'string'

local active = {}
local head, tail = 0, 0
local waiting = {}

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
-- moved from the active queue to the wait list.
yield = coroutine.yield

-- Move the given task from the wait list to the active queue.
function resume(task)
    if waiting[task] then
        waiting[task] = nil
        active[tail] = task
        tail = tail + 1
        signal.wake()
    end
end

-- Run the task dispatch loop.
function main(fn)
    if fn then start(fn) end
    while true do
        -- Dispatch signals and wait for at least one active task.
        signal.dispatch(head == tail)

        -- Resume the task at the head of the active queue.
        local task = active[head]
        active[head] = nil
        head = head + 1
        local _, suspend = coroutine.resume(task)

        -- Close the task if it's dead, park it if it is suspended, or move it
        -- to the end of the active queue.
        if coroutine.status(task) == 'dead' then
            local ok, err = coroutine.close(task)
            if not ok then
                stderr:write(string.format("Task failed: %s\n", err))
            end
        elseif suspend then
            waiting[task] = true
        else
            active[tail] = task
            tail = tail + 1
        end
    end
end
