-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local platform = require 'mlua.platform'

function set_up(t)
    t:printf("Platform: %s\n", platform.name)
end

function test_time_us(t)
    t:expect(t:expr(platform).time_us())
        :gte(platform.min_time):lte(platform.max_time)
    local prev = platform.min_time
    for i = 1, 10 do
        local cur = platform.time_us()
        t:expect(cur):label("time_us()"):gt(prev)
        prev = cur
    end
end

function test_flash(t)
    local flash = platform.flash
    if not flash then t:skip("platform has no flash") end
    t:expect(flash):label("flash")
        :has('address'):has('size'):has('write_size'):has('erase_size')
end
