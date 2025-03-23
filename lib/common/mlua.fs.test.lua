-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

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
        t:expect(t.expr(fs).join(table.unpack(args))):eq(want)
    end
end

function test_split(t)
    for _, test in ipairs{
        {'abc/def/ghi', {'abc/def', 'ghi'}},
        {'abc/def', {'abc', 'def'}},
        {'abc', {'', 'abc'}},
        {'', {'', ''}},
        {'/abc/def/ghi', {'/abc/def', 'ghi'}},
        {'/abc/def', {'/abc', 'def'}},
        {'/abc', {'/', 'abc'}},
        {'/', {'/', ''}},
        {'abc///def', {'abc', 'def'}},
        {'///abc', {'///', 'abc'}},
        {'///', {'///', ''}},
    } do
        local arg, want = table.unpack(test)
        t:expect(t.mexpr(fs).split(arg)):eq(want)
    end
end
