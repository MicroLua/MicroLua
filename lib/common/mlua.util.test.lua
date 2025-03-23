-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local util = require 'mlua.util'
local table = require 'table'

function test_keys(t)
    for _, test in ipairs{
        {{}, nil, {n = 0}},
        {{a = 1, b = 2, c = 3}, nil, {'a', 'b', 'c', n = 3}},
        {{a = 4, b = 5, c = 6}, function(k, v) return k ~= 'b' and v ~= 4 end,
         {'c', n = 1}},
    } do
        local tab, filter, want = table.unpack(test, 1, 3)
        t:expect(t.expr(util).keys(tab, filter):sort()):eq(want, util.table_eq)
    end
end

function test_values(t)
    for _, test in ipairs{
        {{}, nil, {n = 0}},
        {{a = 1, b = 2, c = 3}, nil, {1, 2, 3, n = 3}},
        {{a = 4, b = 5, c = 6}, function(k, v) return k ~= 'b' and v ~= 4 end,
         {6, n = 1}},
    } do
        local tab, filter, want = table.unpack(test, 1, 3)
        t:expect(t.expr(util).values(tab, filter):sort())
            :eq(want, util.table_eq)
    end
end

function test_table_eq(t)
    for _, test in ipairs{
        {nil, nil, true},
        {{}, nil, false},
        {{}, {}, true},
        {{}, {1, 2}, false},
        {{1, 2}, {1, 3}, false},
        {{1, 2}, {1, 2}, true},
        {{a = 1, b = 2}, {a = 1, b = 2}, true},
        {{a = 1, b = 2}, {a = 1, c = 2}, false},
        {{a = 1, b = 2}, {a = 1, b = 3}, false},
    } do
        local a, b, want = table.unpack(test, 1, 3)
        t:expect(t.expr(util).table_eq(a, b)):eq(want)
        t:expect(t.expr(util).table_eq(b, a)):eq(want)
    end
end

function test_table_copy(t)
    for _, test in ipairs{
        {nil},
        {{}},
        {{1, 2, a = 3, b = 4}},
        {setmetatable({1, 2, a = 3, b = 4}, {5, 6})},
    } do
        local tab = table.unpack(test, 1, 1)
        t:expect(t.expr(util).table_copy(tab)):eq(tab, util.table_eq)
            :apply(getmetatable):op("metatable is"):eq(getmetatable(tab))
    end
end

function test_table_comp(t)
    local a, b = {10, 20, 30}, {30, 20, 10}
    for _, test in ipairs{
        {{1}, true}, {{2}, false}, {{3}, false},
        {{1, 2, 3}, true}, {{2, 2, 2}, false}, {{3, 2, 1}, false},
    } do
        local keys, want = table.unpack(test)
        t:expect(t.expr(util).table_comp(keys)(a, b))
    end
end
