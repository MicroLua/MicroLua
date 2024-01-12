-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local coroutine = require 'coroutine'
local event = require 'mlua.event'
local oo = require 'mlua.oo'
local time = require 'mlua.time'
local string = require 'string'

-- Return the currently-running thread.
running = coroutine.running

-- Make the running thread yield. If the argument is true, the thread is moved
-- from the active queue to the wait list. If the argument is an absolute time,
-- the thread is resumed at that time (unless it's resumed explicitly earlier).
yield = coroutine.yield

-- Control if yielding is enabled.
yield_enabled = event.yield_enabled

local co_resume, co_status = coroutine.resume, coroutine.status
local co_close = coroutine.close
local ticks, min_ticks, max_ticks = time.ticks, time.min_ticks, time.max_ticks

local weak_keys = {__mode = 'k'}

local active, head, tail = {}
local waiting = {}
local timers, timers_head = {}, false

-- Add a waiting thread to the timer list.
local function add_timer(thread, deadline)
    if not timers_head or deadline < waiting[timers_head] then
        timers[thread] = timers_head
        timers_head = thread
        return
    end
    local prev = timers_head
    while true do
        local next = timers[prev]
        if not next or deadline < waiting[next] then
            timers[thread] = next
            timers[prev] = thread
            return
        end
        prev = next
    end
end

-- Remove a waiting thread from the timer list.
local function remove_timer(thread)
    if thread == timers_head then
        timers_head = timers[thread]
        timers[thread] = nil
        return
    end
    local prev = timers_head
    while true do
        local next = timers[prev]
        if not next then return end
        if next == thread then
            timers[prev] = timers[thread]
            timers[thread] = nil
            return
        end
        prev = next
    end
end

-- Make threads a class.
local Thread = {__name = 'Thread'}
Thread.__index = Thread
event.set_thread_metatable(Thread)

-- Start a new thread calling the given function.
function Thread.start(fn)
    local thread = coroutine.create(fn)
    if tail then active[tail] = thread else head = thread end
    tail = thread
    return thread
end

start = Thread.start

local _shutdown, _shutdown_res = false

-- Shut down the scheduler.
function Thread.shutdown(result)
    _shutdown, _shutdown_res = true, result
    yield()
end

shutdown = Thread.shutdown

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
    if tail then active[tail] = self else head = self end
    tail = self
    return true
end

local terms = setmetatable({}, weak_keys)
local joiners = setmetatable({}, weak_keys)

-- Kill the thread.
function Thread:kill()
    local deadline = waiting[self]
    if deadline and deadline ~= true then remove_timer(self) end
    waiting[self] = nil
    local ok, err = co_close(self)
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
        repeat yield(true) until co_status(self) == 'dead'
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
    -- TODO: Allow adding new threads while the group is being joined
    -- TODO: Wait for all threads even if one of them throws
    for thread in pairs(self) do thread:join() end
end

-- Join the threads in the group on closure.
Group.__close = Group.join

-- Resume the threads whose deadline has elapsed.
local function resume_deadlined()
    local t = ticks()
    while timers_head do
        if waiting[timers_head] > t then return end
        waiting[timers_head] = nil
        if tail then active[tail] = timers_head else head = timers_head end
        tail = timers_head
        timers_head, timers[timers_head] = timers[timers_head], nil
    end
end

-- Run the thread dispatch loop.
function main()
    local thread
    local cleanup<close> = function()
        if thread then co_close(thread) end
        while head do
            co_close(head)
            head = active[head]
        end
        for thread in pairs(waiting) do co_close(thread) end
        active, head, tail, waiting = nil, nil, nil, nil
        timers, timers_head = nil, nil
    end
    local dispatch = event.dispatch

    while not _shutdown do
        -- Dispatch events and wait for at least one active thread.
        dispatch((thread or head) and min_ticks or waiting[timers_head]
                 or max_ticks)

        -- Resume threads whose deadline has elapsed.
        resume_deadlined()

        -- If the previous running thread is still active, move it to the end of
        -- the active queue, after threads resumed by events. Then get the
        -- thread at the head of the active queue, skipping killed threads.
        if head then
            if thread then active[tail], tail = thread, thread end
            repeat
                thread = head
                head = active[head]
                if not head then tail = nil end
                active[thread] = nil
                if co_status(thread) == 'dead' then thread = nil end
            until thread or head == tail
        end

        -- Resume the next thread, then close it if it's dead, or move it to the
        -- wait list if it's suspended.
        if thread then
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
    return _shutdown_res
end
