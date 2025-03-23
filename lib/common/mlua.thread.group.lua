-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local oo = require 'mlua.oo'
local thread = require 'mlua.thread'

local start = thread.start

-- A group of threads managed together.
Group = oo.class('Group')
Group.__mode = 'k'

-- Start a new thread and track it in the group.
function Group:start(fn, name)
    local th = start(fn, name)
    self[th] = true
    return th
end

-- Join all threads in the group.
function Group:join()
    -- TODO: Allow adding new threads while the group is being joined
    -- TODO: Wait for all threads even if one of them throws
    for th in pairs(self) do th:join() end
end

-- Join the threads in the group on closure.
Group.__close = Group.join

thread.Group = Group
