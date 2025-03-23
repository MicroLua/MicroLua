-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local int64 = require 'mlua.int64'
local rand = require 'pico.rand'

local function check(t, fn, n, want)
    -- This isn't a proper randomness test, just a quick check to make sure the
    -- generated values vary at least somewhat.
    local _helper = t.helper
    local values = {}
    for i = 1, n do
        local v1, v2 = fn()
        values[tostring(v1)] = true
        if v2 then values[tostring(v2)] = true end
    end
    local count = 0
    for v in pairs(values) do count = count + 1 end
    t:expect(count):label("distinct values"):gte(want)
end

function test_get_rand_32(t)
    check(t, rand.get_rand_32, 100, 98)
end

function test_get_rand_64(t)
    check(t, rand.get_rand_64, 100, 98)
end

function test_get_rand_128(t)
    check(t, rand.get_rand_128, 50, 98)
end
