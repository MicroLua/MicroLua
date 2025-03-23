-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local list = require 'mlua.list'
local repr = require 'mlua.repr'
local table = require 'table'

function test_call_args(t)
    local A = {}
    function A.__call(...) return list.pack(...) end
    local a = setmetatable({}, A)

    local got = a(1, 2)
    t:expect(a(1, 2)):label('a(1, 2)'):eq({a, 1, 2})

    local b = {a = a}
    t:expect(b.a(1, 2)):label('b.a(1, 2)'):eq({a, 1, 2})
    t:expect(b:a(1, 2)):label('b:a(1, 2)'):eq({a, b, 1, 2})
end

local v = {13}

function f1(a, b)
    return {f2 = function(c, d) return a + b + c + d end, [v] = 42}
end

local function f3(a, b) return a + b end

function test_Expr(t)
    local _ = f3  -- Capture the upvalue
    local function f4(a, b) return a * b end
    local c = {
        a = 7,
        f = function(self, b) return self.a + b end,
        g = function(s1, x)
            return {b = 8, h = function(s2, y) return s1.a + x + s2.b + y end}
        end,
    }

    for _, test in ipairs{
        {t.expr.v[1], 'v[1]', 13},
        {t.expr.f1(1, 2).f2(3, 4), 'f1(1, 2).f2(3, 4)', 10},
        {t.expr.f3(5, 6), 'f3(5, 6)', 11},
        {t.expr.f4(7, 8), 'f4(7, 8)', 56},
        {t.expr.c:f(9), 'c:f(9)', 16},
        {t.expr(c):f(10), 'f(10)', 17},
        {t.expr.c:g(11):h(12), 'c:g(11):h(12)', 38},
        {t.expr(c):g(11):h(12), 'g(11):h(12)', 38},
        {t.expr(_ENV).f1(1, 2).f2(3, 4), 'f1(1, 2).f2(3, 4)', 10},
        {t.expr(_ENV).f1(5)[v], 'f1(5)[{13}]', 42},
    } do
        local e, want_repr, want_eval = table.unpack(test)
        t:expect(repr(e)):label('repr()'):eq(want_repr)
        t:expect(t.expr(getmetatable(e)).__eval(e)):eq(want_eval)
    end
end
