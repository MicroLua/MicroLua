_ENV = mlua.Module(...)

local util = require 'mlua.util'
local table = require 'table'

function test_repr(t)
    for _, test in ipairs{
        {nil, 'nil'},
        {123, '123'},
        {4.5, '4.500000'},
        {'abc\ndef', '"abc\\ndef"'},
        {true, 'true'},
        {{}, '{}'},
        {{1, 2}, '{[1] = 1, [2] = 2}'},
        {{a = 1, b = 2}, '{["a"] = 1, ["b"] = 2}'},
        {{a = {1, 2}, b = {c = 3, d = 4}},
         '{["a"] = {[1] = 1, [2] = 2}, ["b"] = {["c"] = 3, ["d"] = 4}}'},
    } do
        local v, want = table.unpack(test)
        t:expect(t:expr(util).repr(v)):eq(want)
    end
end

function test_keys(t)
    for _, test in ipairs{
        {{}, nil, {[0] = 0}},
        {{a = 1, b = 2, c = 3}, nil, {[0] = 3, 'a', 'b', 'c'}},
        {{a = 4, b = 5, c = 6}, function(k, v) return k ~= 'b' and v ~= 4 end,
         {[0] = 1, 'c'}},
    } do
        local tab, filter, want = table.unpack(test)
        t:expect(t:expr(util).keys(tab, filter)):fix(util.sort)
            :eq(want, util.table_eq)
    end
end

function test_values(t)
    for _, test in ipairs{
        {{}, nil, {[0] = 0}},
        {{a = 1, b = 2, c = 3}, nil, {[0] = 3, 1, 2, 3}},
        {{a = 4, b = 5, c = 6}, function(k, v) return k ~= 'b' and v ~= 4 end,
         {[0] = 1, 6}},
    } do
        local tab, filter, want = table.unpack(test)
        t:expect(t:expr(util).values(tab, filter)):fix(util.sort)
            :eq(want, util.table_eq)
    end
end

function test_sort(t)
    for _, test in ipairs{
        {{}, nil, {}},
        {{1, 4, 2, 8, 5}, nil, {1, 2, 4, 5, 8}},
        {{[0] = 5, 1, 4, 2, 8, 5}, nil, {[0] = 5, 1, 2, 4, 5, 8}},
        {{1, 4, 2, 8, 5}, function(a, b) return a > b end, {8, 5, 4, 2, 1}},
    } do
        local items, comp, want = table.unpack(test)
        t:expect(t:expr(util).sort(items, comp)):eq(want, util.table_eq)
    end
end

function test_table_eq(t)
    for _, test in ipairs{
        {{}, {}, true},
        {{1, 2}, {}, false},
        {{}, {1, 2}, false},
        {{1, 2}, {1, 3}, false},
        {{1, 2}, {1, 2}, true},
        {{a = 1, b = 2}, {a = 1, b = 2}, true},
        {{a = 1, b = 2}, {a = 1, c = 2}, false},
        {{a = 1, b = 2}, {a = 1, b = 3}, false},
    } do
        local a, b, want = table.unpack(test)
        t:expect(t:expr(util).table_eq(a, b)):eq(want)
    end
end

function test_table_comp(t)
    local a, b = {10, 20, 30}, {30, 20, 10}
    for _, test in ipairs{
        {{1}, true}, {{2}, false}, {{3}, false},
        {{1, 2, 3}, true}, {{2, 2, 2}, false}, {{3, 2, 1}, false},
    } do
        local keys, want = table.unpack(test)
        t:expect(t:expr(util).table_comp(keys)(a, b))
    end
end
