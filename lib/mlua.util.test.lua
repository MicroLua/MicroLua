_ENV = mlua.Module(...)

local util = require 'mlua.util'
local table = require 'table'

function test_str(t)
    for _, test in ipairs{
        {nil, 'nil'},
        {123, '123'},
        {4.5, '4.500000'},
        {'abc', '"abc"'},
        {true, 'true'},
        {{}, '{}'},
        {{1, 2}, '{[1] = 1, [2] = 2}'},
        {{a = 1, b = 2}, '{["a"] = 1, ["b"] = 2}'},
        {{a = {1, 2}, b = {c = 3, d = 4}},
         '{["a"] = {[1] = 1, [2] = 2}, ["b"] = {["c"] = 3, ["d"] = 4}}'},
    } do
        local v, want = table.unpack(test)
        local got = util.str(v)
        t:expect(got == want, "str(%s) = %s, want %s", v, got, want)
        local got = tostring(util.lstr(v))
        t:expect(got == want, "lstr(%s) = %s, want %s", v, got, want)
    end
end

function test_len(t)
    for _, test in ipairs{
        {nil, 0},
        {{}, 0},
        {{1, 2, 3}, 3},
        {{[0] = 7}, 7},
    } do
        local list, want = table.unpack(test)
        local got = util.len(list)
        t:expect(got == want,
                 "len(%s) = %s, want %s", util.lstr(list), got, want)
    end
end

function test_append(t)
    for _, test in ipairs{
        {nil, 1, {[0] = 1, 1}},
        {{}, 1, {[0] = 1, 1}},
        {{1, 2}, 3, {[0] = 3, 1, 2, 3}},
        {{[0] = 2, 1, 2}, 3, {[0] = 3, 1, 2, 3}},
    } do
        local list, el, want = table.unpack(test)
        local got = util.append(list, el)
        t:expect(util.table_eq(got, want),
                 "append(%s) = %s, want %s", util.lstr(list), util.lstr(got),
                 util.lstr(want))
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
        local got = util.keys(tab, filter)
        table.sort(got)
        t:expect(util.table_eq(got, want),
                 "keys(%s) = %s, want %s", util.lstr(tab), util.lstr(got),
                 util.lstr(want))
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
        local got = util.values(tab, filter)
        table.sort(got)
        t:expect(util.table_eq(got, want),
                 "values(%s) = %s, want %s", util.lstr(tab), util.lstr(got),
                 util.lstr(want))
    end
end

function test_sort(t)
    for _, test in ipairs{
        {{}, nil, {}},
        {{1, 4, 2, 8, 5}, nil, {1, 2, 4, 5, 8}},
        {{[0] = 5, 1, 4, 2, 8, 5}, nil, {[0] = 5, 1, 2, 4, 5, 8}},
        {{1, 4, 2, 8, 5}, function(a, b) return a > b end, {8, 5, 4, 2, 1}},
    } do
        local list, comp, want = table.unpack(test)
        local got = util.sort(list, comp)
        t:expect(util.table_eq(got, want),
                 "sort(%s) = %s, want %s", util.lstr(list), util.lstr(got),
                 util.lstr(want))
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
        local got = util.table_eq(a, b)
        t:expect(got == want, "table_eq(%s, %s) = %s, want %s", util.lstr(a),
                 util.lstr(b), got, want)
    end
end

function test_table_comp(t)
    local a, b = {10, 20, 30}, {30, 20, 10}
    for _, test in ipairs{
        {{1}, true}, {{2}, false}, {{3}, false},
        {{1, 2, 3}, true}, {{2, 2, 2}, false}, {{3, 2, 1}, false},
    } do
        local keys, want = table.unpack(test)
        local got = util.table_comp(keys)(a, b)
        t:expect(got == want,
                 "table_comp(%s) = %s, want %s", util.lstr(keys), got, want)
    end
end
