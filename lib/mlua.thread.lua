-- A cooperative threading library.

_ENV = mlua.Module(...)

local coroutine = require 'coroutine'
local event = require 'mlua.event'
local oo = require 'mlua.oo'
local time = require 'pico.time'
local string = require 'string'
local table = require 'table'

-- Return the currently-running thread.
running = coroutine.running

-- Make the running thread yield. If the argument is true, the thread is moved
-- from the active queue to the wait list. If the argument is an absolute time,
-- the thread is resumed at that time (unless it's resumed explicitly earlier).
yield = coroutine.yield

local co_resume, co_status = coroutine.resume, coroutine.status
local co_close = coroutine.close

local weak_keys = {__mode = 'k'}

local active = {}
local head, tail = 0, 0
local waiting = {}
local timers = {[0] = 0}

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

local _shutdown = false

-- Shut down the scheduler.
function shutdown()
    _shutdown = true
    yield()
end

-- Make threads a class.
local Thread = {__name = 'Thread'}
Thread.__index = Thread
event.set_thread_metatable(Thread)

-- Start a new thread calling the given function.
function Thread.start(fn)
    local thread = coroutine.create(fn)
    active[tail] = thread
    tail = tail + 1
    return thread
end

start = Thread.start

local names = setmetatable({}, weak_keys)

-- Return the name of the thread.
function Thread:name() return names[self] or tostring(self):sub(9) end

-- Set the name of the thread.
function Thread:set_name(name)
    names[self] = name
    return self
end

-- Return true iff the thread is alive.
function Thread:is_alive() return co_status(self) ~= 'dead' end

-- Return true iff the thread is on the wait list.
function Thread:is_waiting() return waiting[self] ~= nil end

-- Move the thread from the wait list to the active queue.
function Thread:resume()
    local deadline = waiting[self]
    if not deadline then return false end
    if deadline ~= true then remove_timer(self) end
    waiting[self] = nil
    active[tail] = self
    tail = tail + 1
    return true
end

local terms = setmetatable({}, weak_keys)
local joiners = setmetatable({}, weak_keys)

-- Kill the thread.
function Thread:kill()
    local ok, err = co_close(self)
    waiting[self] = nil
    if not ok then terms[self] = err end
    local js = joiners[self]
    if js then
        joiners[self] = nil
        if type(js) ~= 'table' then js:resume()
        else for _, j in ipairs(js) do j:resume() end end
    end
end

-- Block until the thread has terminated.
function Thread:join()
    if co_status(self) ~= 'dead' then
        local j = running()
        local js = joiners[self]
        if not js then
            joiners[self] = j
        elseif type(js) ~= 'table' then
            joiners[self] = {[js] = true, [j] = true}
        else
            js[j] = true
        end
        repeat yield() until co_status(self) == 'dead'
    end
    local term = terms[self]
    if term then error(term, 0) end
end

-- Join the thread on closure.
Thread.__close = Thread.join

-- A group of threads managed together.
Group = oo.class('Group')
Group.__mode = 'k'

-- Start a new thread and track it in the group.
function Group:start(fn)
    local thread = start(fn)
    self[thread] = true
    return thread
end

-- Join all threads in the group.
function Group:join()
    for thread in pairs(self) do thread:join() end
end

-- Join the threads in the group on closure.
Group.__close = Group.join

-- Return the current absolute time.
now = time.get_absolute_time

-- Suspend the running thread until the given absolute time.
sleep_until = time.sleep_until

-- Suspend the running thread for the given duration (in microseconds).
sleep_for = time.sleep_us

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
function main()
    local thread
    local cleanup<close> = function()
        if thread then co_close(thread) end
        while head ~= tail do
            co_close(active[head])
            head = head + 1
        end
        for thread in pairs(waiting) do co_close(thread) end
        active, head, tail, waiting, timers = nil, nil, nil, nil, nil
    end
    local resume = Thread.resume
    local nil_time, at_the_end_of_time = time.nil_time, time.at_the_end_of_time

    while not _shutdown do
        -- Dispatch events and wait for at least one active thread.
        event.dispatch(resume,
                       (thread ~= nil or head ~= tail) and nil_time
                       or waiting[timers[1]] or at_the_end_of_time)

        -- Resume threads whose deadline has elapsed.
        resume_deadlined()

        -- If the previous running thread is still active, move it to the end of
        -- the active queue, after threads resumed by events. Then get the
        -- thread at the head of the active queue, skipping killed threads.
        if head ~= tail then
            if thread then
                active[tail] = thread
                tail = tail + 1
            end
            repeat
                thread = active[head]
                active[head] = nil
                head = head + 1
            until co_status(thread) ~= 'dead'
        end

        -- Resume the next thread, then close it if it's dead, or move it to the
        -- wait list if it's suspended.
        local _, deadline = co_resume(thread)
        if co_status(thread) == 'dead' then
            thread:kill()
            thread = nil
        elseif deadline then
            waiting[thread] = deadline
            if deadline ~= true then add_timer(thread, deadline) end;
            thread = nil
        end
    end
end
