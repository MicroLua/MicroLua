_ENV = mlua.Module(...)

local util = require 'mlua.util'
local table = require 'table'

local v = {13}

function f1(a, b)
    return {f2 = function(c, d) return a + b + c + d end, [v] = 42}
end

function test_Expr(t)
    for _, test in ipairs{
        {t:expr(_ENV).f1(1, 2).f2(3, 4), 'f1(1, 2).f2(3, 4)', 10},
        {t:expr(_ENV).f1(5)[v], 'f1(5)[{[1] = 13}]', 42},
    } do
        local e, want_repr, want_eval = table.unpack(test)
        t:expect(util.repr(e)):label('repr()'):eq(want_repr)
        t:expect(t:expr(getmetatable(e)).__eval(e)):eq(want_eval)
    end
end
