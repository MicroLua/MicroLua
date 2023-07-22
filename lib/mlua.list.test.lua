_ENV = mlua.Module(...)

local list = require 'mlua.list'
local util = require 'mlua.util'
local table = require 'table'

function test_list(t)
    local list = list
    for _, test in ipairs{
        {nil, {[0] = 0}},
        {{1, 2, 3}, {[0] = 3, 1, 2, 3}},
        {{[0] = 2}, {[0] = 2}},
    } do
        local arg, want = table.unpack(test)
        t:expect(t.expr.list(arg)):eq(want)
    end
end

function test_len(t)
    for _, test in ipairs{
        {nil, 0},
        {{}, 0},
        {{1, 2, 3}, 3},
        {{[0] = 7}, 7},
    } do
        local arg, want = table.unpack(test)
        t:expect(t:expr(list).len(arg)):eq(want)
    end
end

function test_eq(t)
    for _, test in ipairs{
        {nil, nil, true},
        {nil, {}, true},
        {{1, 2}, {1, 2, 3}, false},
        {{1, 2, 3}, {1, 2, 3}, true},
        {{1, 2, 3}, {1, 2, 4}, false},
        {{[0] = 3, 1, nil, 3}, {[0] = 3, 1, nil, 3}, true},
        {{[0] = 3, 1, nil, 3}, {[0] = 3, 1, nil, 4}, false},
        {{1, 2, 3, a = 4}, {1, 2, 3, b = 5}, true},
    } do
        local a, b, want = table.unpack(test)
        t:expect(t:expr(list).eq(a, b)):eq(want)
        t:expect(t:expr(list).eq(b, a)):eq(want)
    end
end

function test_ipairs(t)
    for _, test in ipairs{
        {nil, {}, {}},
        {{}, {}, {}},
        {{10, 20, 30}, {1, 2, 3}, {10, 20, 30}},
        {{[0] = 7}, {1, 2, 3, 4, 5, 6, 7}, {[0] = 7}},
    } do
        local arg, wis, wvs = table.unpack(test)
        local is, vs = list(), list()
        for i, v in list.ipairs(arg) do
            is:append(i)
            vs:append(v)
        end
        t:expect(is == wis,
                 "ipairs(%s) indexes %s, want %s", t:repr(arg), t:repr(is),
                 t:repr(wis))
        t:expect(vs == wvs,
                 "ipairs(%s) values %s, want %s", t:repr(arg), t:repr(vs),
                 t:repr(wvs))
    end
end

function test_append(t)
    for _, test in ipairs{
        {nil, {1, 2}, {[0] = 2, 1, 2}},
        {{}, {3, 4, 5}, {[0] = 3, 3, 4, 5}},
        {{1, 2}, {3, 4}, {[0] = 4, 1, 2, 3, 4}},
        {{[0] = 2, 1, 2}, {3}, {[0] = 3, 1, 2, 3}},
        {nil, {}, {[0] = 0}},
        {{1, 2}, {}, {[0] = 2, 1, 2}},
    } do
        local arg, els, want = table.unpack(test)
        t:expect(t:expr(list).append(arg, list.unpack(els)))
            :eq(want, util.table_eq)
    end
end

function test_pack(t)
    for _, test in ipairs{
        {list.pack(), list{[0] = 0}},
        {list.pack(1, 2, 3), list{[0] = 3, 1, 2, 3}},
        {list.pack(1, nil, 3), list{[0] = 3, 1, nil, 3}},
        {list.pack(nil), list{[0] = 1}},
        {list.pack(nil, nil, nil), list{[0] = 3}},
    } do
        local got, want = table.unpack(test)
        t:expect(got):label("pack()"):eq(want)
    end
end

function test_unpack(t)
    for _, test in ipairs{
        {{nil}, nil, nil, nil},
        {{{}}, nil, nil, nil},
        {{{1, 2, 3}}, 1, 2, 3},
        {{list.pack(nil, 2, 3)}, nil, 2, 3},
        -- TODO: Add test cases with index arguments
    } do
        local args, wa, wb, wc = table.unpack(test)
        -- TODO: Test round-tripping instead of using multiple assignment
        local a, b, c = list.unpack(table.unpack(args))
        t:expect(a == wa and b == wb and c == wc,
                 "%s = (%s, %s, %s), want (%s, %s, %s)", t:func('unpack', args),
                 t:repr(a), t:repr(b), t:repr(c),
                 t:repr(wa), t:repr(wb), t:repr(wc))
    end
end

function test_repr(t)
    for _, test in ipairs{
        {list.pack(), '{}'},
        {list.pack(1), '{1}'},
        {list.pack(1, "2", 3, nil), '{1, "2", 3, nil}'},
    } do
        local arg, want = table.unpack(test)
        t:expect(t:expr(util).repr(arg)):eq(want)
    end
end
