-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local thread = require 'mlua.thread'
local time = require 'mlua.time'
local multicore = require 'pico.multicore'
local table = require 'table'

local module_name = ...

function test_core1(t)
    for _, test in ipairs{
        {{module_name, 'core1_exit'}, nil},
        {{module_name, 'core1_exit'}, 2 * time.msec},
        {{module_name, 'core1_suspend'}, nil},
        {{module_name, 'core1_suspend'}, 2 * time.msec},
        {{module_name, 'core1_busy'}, 2 * time.msec},
    } do
        local args, sleep = table.unpack(test)
        multicore.launch_core1(table.unpack(args))
        if sleep then time.sleep_for(sleep) end
        multicore.reset_core1()
    end
end

function core1_exit()
    thread.shutdown()
end

function core1_suspend()
    multicore.set_shutdown_handler()
    thread.suspend()
end

function core1_busy()
    multicore.set_shutdown_handler()
    while true do thread.yield() end
end
