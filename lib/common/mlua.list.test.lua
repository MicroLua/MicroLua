-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local list = require 'mlua.list'
local repr = require 'mlua.repr'
local util = require 'mlua.util'
local table = require 'table'

function test_list(t)
    local list = list
    for _, test in ipairs{
        {nil, {n = 0}},
        {{1, 2, 3}, {1, 2, 3, n = 3}},
        {{n = 2}, {n = 2}},
    } do
        local arg, want = table.unpack(test)
        t:expect(t.expr.list(arg)):eq(want)
    end
end

function test_len(t)
    for _, test in ipairs{
        {nil, nil, 0, nil},
        {{}, nil, 0, {}},
        {{1, 2, 3}, nil, 3, {1, 2, 3}},
        {{n = 7}, nil, 7, {n = 7}},
        {nil, 0, 0, nil},
        {{}, 0, 0, {}},
        {{1, 2, 3}, 5, 3, {1, 2, 3, n = 5}},
        {{1, 2, 3, 4, 5}, 3, 5, {1, 2, 3, n = 3}},
        {{n = 7}, 4, 7, {n = 4}},
    } do
        local arg, new, want, want_list = table.unpack(test, 1, 4)
        t:expect(t.expr(list).len(arg, new)):eq(want)
        t:expect(arg):label("list"):eq(want_list, util.table_eq)
    end
end

function test_eq(t)
    for _, test in ipairs{
        {nil, nil, true},
        {nil, {}, true},
        {{1, 2}, {1, 2, 3}, false},
        {{1, 2, 3}, {1, 2, 3}, true},
        {{1, 2, 3}, {1, 2, 4}, false},
        {{1, nil, 3, n = 3}, {1, nil, 3, n = 3}, true},
        {{1, nil, 3, n = 3}, {1, nil, 4, n = 3}, false},
        {{1, 2, 3, a = 4}, {1, 2, 3, b = 5}, true},
    } do
        local a, b, want = table.unpack(test, 1, 3)
        t:expect(t.expr(list).eq(a, b)):eq(want)
        t:expect(t.expr(list).eq(b, a)):eq(want)
    end
end

function test_ipairs(t)
    for _, test in ipairs{
        {nil, {}, {}},
        {{}, {}, {}},
        {{10, 20, 30}, {1, 2, 3}, {10, 20, 30}},
        {{n = 7}, {1, 2, 3, 4, 5, 6, 7}, {n = 7}},
    } do
        local arg, wis, wvs = table.unpack(test)
        local is, vs = list(), list()
        for i, v in list.ipairs(arg) do
            is:append(i)
            vs:append(v)
        end
        t:expect(is):func("ipairs", arg):op("indexes ="):eq(wis)
        t:expect(vs):func("ipairs", arg):op("values ="):eq(wvs)
    end
end

local function copy_args(args)
    local res = util.table_copy(args)
    res[1] = util.table_copy(args[1])
    return res
end

function test_append(t)
    for _, test in ipairs{
        {{nil, 1, 2, n = 3}, {1, 2, n = 2}},
        {{{}, 3, 4, 5}, {3, 4, 5, n = 3}},
        {{{1, 2}, 3, 4}, {1, 2, 3, 4, n = 4}},
        {{{1, 2, n = 2}, 3}, {1, 2, 3, n = 3}},
        {{}, nil},
        {list.pack(nil), nil},
        {{{1, 2}}, {1, 2}},
    } do
        local args, want = table.unpack(test)
        local argsc = copy_args(args)
        t:expect(list.append(table.unpack(args, 1, args.n)))
            :func("append", table.unpack(argsc, 1, argsc.n))
            :eq(want, util.table_eq)
    end
end

function test_insert(t)
    for _, test in ipairs{
        {{nil, 1, n = 2}, {1, n = 1}},
        {{{}, 2}, {2, n = 1}},
        {{{1, 2, 3}, 4}, {1, 2, 3, 4, n = 4}},
        {{{1, 2, 3}, 2, 42}, {1, 42, 2, 3, n = 4}},
    } do
        local args, want = table.unpack(test)
        local argsc = copy_args(args)
        t:expect(list.insert(table.unpack(args, 1, args.n)))
            :func("insert", table.unpack(argsc, 1, argsc.n))
            :eq(want, util.table_eq)
    end
end

function test_remove(t)
    for _, test in ipairs{
        {{nil, 0, n = 2}, nil, nil},
        {{{}, 0}, nil, {}},
        {{{1, 2, 3, 4}, nil}, 4, {1, 2, 3, n = 3}},
        {{{1, 2, 3, 4}, 2}, 2, {1, 3, 4, n = 3}},
    } do
        local args, want_res, want = table.unpack(test, 1, 3)
        local argsc = copy_args(args)
        t:expect(list.remove(table.unpack(args, 1, args.n)))
            :func("remove", table.unpack(argsc, 1, argsc.n)):eq(want_res)
        local eq = want ~= nil and util.table_eq or nil
        t:expect(args[1]):func("remove", table.unpack(argsc, 1, argsc.n))
            :op("->"):eq(want, eq)
    end
end

function test_pack(t)
    for _, test in ipairs{
        {list.pack(), list{n = 0}},
        {list.pack(1, 2, 3), list{1, 2, 3, n = 3}},
        {list.pack(1, nil, 3), list{1, nil, 3, n = 3}},
        {list.pack(nil), list{n = 1}},
        {list.pack(nil, nil, nil), list{n = 3}},
    } do
        local got, want = table.unpack(test)
        t:expect(got):label("pack()"):eq(want)
    end
end

function test_unpack(t)
    for _, test in ipairs{
        {{nil, n = 1}, {}},
        {{{}}, {}},
        {{{1, 2, 3}}, {1, 2, 3}},
        {{{nil, 2, 3}}, {nil, 2, 3}},
        {{{1, 2, 3, 4, 5}, 3}, {3, 4, 5}},
        {{{1, 2, 3, 4, 5}, nil, 4, n = 3}, {1, 2, 3, 4}},
        {{{1, 2, 3, 4, 5}, 2, 3}, {2, 3}},
        {{{1, 2, 3, 4, 5}, 4, 3}, {}},
    } do
        local args, want = table.unpack(test)
        local got = list.pack(list.unpack(table.unpack(args, 1, args.n)))
        t:expect(got):func("unpack", table.unpack(args, 1, args.n))
            :eq(want, list.eq)
    end
end

function test_sort(t)
    for _, test in ipairs{
        {{nil, n = 1}, nil},
        {{{}}, {}},
        {{{1, 4, 2, 8, 5, 7}}, {1, 2, 4, 5, 7, 8}},
        {{{1, 4, 2, 8, 5}, function(a, b) return a > b end}, {8, 5, 4, 2, 1}},
        {
            {
                {1, 4, 2, 8, n = 7},
                function(a, b)
                    if a == nil then return b ~= nil end
                    if b == nil then return false end
                    return a < b
                end,
            },
            {nil, nil, nil, 1, 2, 4, 8, n = 7},
        },
    } do
        local args, want = table.unpack(test, 1, 2)
        t:expect(t.expr(list).sort(table.unpack(args, 1, args.n)))
            :eq(want, list.eq)
    end
end

function test_concat(t)
    for _, test in ipairs{
        {{nil, ','}, ''},
        {{{}, ','}, ''},
        {{{'a', 'b', 'c'}, ''}, 'abc'},
        {{{'a', 'b', 'c'}, ','}, 'a,b,c'},
        {{{'a', 'b', 'c'}, ',', 2}, 'b,c'},
        {{{'a', 'b', 'c', 'd'}, ',', 2, 3}, 'b,c'},
        {{{'a', 'b', 'c', 'd'}, ',', 3, 2}, ''},
    } do
        local args, want = table.unpack(test)
        t:expect(t.expr(list).concat(table.unpack(args))):eq(want)
    end
end

function test_find(t)
    for _, test in ipairs{
        {{nil, 1, n = 2}, nil},
        {{{}, 1}, nil},
        {{{1, 2, 3, 1, 2}, 4}, nil},
        {{{1, 2, 3, 1, 2}, 2}, 2},
        {{{1, 2, 3, 1, 2}, 2, 0}, 2},
        {{{1, 2, 3, 1, 2}, 2, 3}, 5},
        {{{1, 2, 3, 1, 2}, 2, 10}, nil},
        {{{1, 2, 3, 1, 2}, 2, -2}, 5},
        {{{1, 2, 3, 1, 2}, 2, -4}, 2},
        {{{1, 2, 3, 1, 2}, 2, -10}, 2},
        {{{[5] = 'a', n = 7}, 'a'}, 5},
    } do
        local args, want = table.unpack(test)
        t:expect(t.expr(list).find(table.unpack(args, 1, args.n))):eq(want)
    end
end

function test_repr(t)
    local rec = list{1, list{2}, 3}
    rec[2]:append(rec)
    rec:append(rec)
    for _, test in ipairs{
        {list.pack(), '{}'},
        {list.pack(1), '{1}'},
        {list.pack(1, "2", 3, nil), '{1, "2", 3, nil}'},
        {rec, '{1, {2, ...}, 3, ...}'},
    } do
        local arg, want = table.unpack(test)
        t:expect(repr(arg)):label("repr()"):eq(want)
    end
end
