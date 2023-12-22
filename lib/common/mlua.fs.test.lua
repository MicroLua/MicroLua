-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local fs = require 'mlua.fs'
local table = require 'table'

function test_join(t)
    for _, test in ipairs{
        {{}, ''},
        {{'', 'abc'}, 'abc'},
        {{'/', 'abc'}, '/abc'},
        {{'abc'}, 'abc'},
        {{'abc', 'def', '', '', 'ghi', ''}, 'abc/def/ghi/'},
        {{'abc', 'def/', 'ghi//', 'jkl/'}, 'abc/def/ghi//jkl/'},
        {{'abc', 'def', '/ghi', 'jkl'}, '/ghi/jkl'},
    } do
        local args, want = table.unpack(test)
        t:expect(t:expr(fs).join(table.unpack(args))):eq(want)
    end
end
