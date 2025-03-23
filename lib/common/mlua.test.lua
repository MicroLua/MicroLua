-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local config = require 'mlua.config'
local io = require 'mlua.io'
local mem = require 'mlua.mem'
local thread = require 'mlua.thread'
local table = require 'table'

local module_name = ...

function set_up(t)
    t:printf("Lua: %s\n", _RELEASE:gsub('[^ ]+ (.*)', '%1'))
end

function test_globals(t)
    t:expect(_G):label("_G"):has('_VERSION')
    t:expect(_G):label("_G"):has('_RELEASE')
    t:expect(t.expr(_G)._VERSION_NUM):gte(504)
    t:expect(t.expr(_G)._RELEASE_NUM):gte(50406)
end

function test_try(t)
    t:expect(t.mexpr(_G).try(function(a, b, c) return c, b, a end, 1, 2, 3))
        :label("success"):eq{3, 2, 1}
    t:expect(t.mexpr(_G).try(function() error("boom", 0) end))
        :label("failure"):eq{nil, "boom", n = 2}
    t:expect(t.mexpr(_G).try(function() thread.yield() return 1, 2 end))
        :label("yielding"):eq{1, 2}
end

function test_equal(t)
    local tab = setmetatable({}, {__eq = function() return true end})
    t:expect(t.expr(_G).equal(tab, 1)):eq(true)
    t:expect(t.expr(_G).equal(1, tab)):eq(true)
end

function test_alloc_stats(t)
    local count1, size1, used1, peak1 = alloc_stats()
    local a = 'abc' .. 'def'
    local count2, size2, used2, peak2 = alloc_stats()
    t:expect(count2 - count1):label("count delta"):gt(0):lt(10)
    t:expect(size2 - size1):label("size delta"):gt(0):lt(1000)
    t:expect(peak1):label("peak1"):gte(used1)
    t:expect(peak2):label("peak2"):gte(used2)
    t:expect(peak2):label("peak2"):gte(peak1)
end

function test_with_traceback(t)
    for _, test in ipairs{
        {function(a, b, c) return c, b, a end, {1, 2, 3}, {3, 2, 1}, nil},
        {function() error("boom", 0) end, {}, nil, "^boom\nstack traceback:\n"},
    } do
        local fn, args, res, err = table.unpack(test, 1, 4)
        local wfn = with_traceback(fn)
        local exp = t:expect(t.mexpr.wfn(table.unpack(args)))
        if not err then exp:eq(res) else exp:raises(err) end
    end
end

function test_log_error(t)
    local r = t:patch(_G, 'stderr', io.Recorder())

    local wfn = log_error(function(a, b, c) return c, b, a end)
    t:expect(t.mexpr.wfn(1, 2, 3)):eq{3, 2, 1}
    t:expect(t.expr(r):is_empty()):eq(true)

    wfn = log_error(function(a, b, c) thread.yield() return b, c, a end)
    t:expect(t.mexpr.wfn(2, 3, 4)):eq{3, 4, 2}
    t:expect(t.expr(r):is_empty()):eq(true)

    wfn = log_error(function(e) error(e) end)
    t:expect(t.expr.wfn("boom")):raises("boom")
    t:expect(tostring(r)):label("stderr")
        :matches("^ERROR: mlua%.test:%d+: boom\nstack traceback:\n")

    local r2 = io.Recorder()
    wfn = log_error(function(e) error(e) end, r2)
    t:expect(t.expr.wfn(123456)):raises(123456)
    t:expect(tostring(r2)):label("r2")
        :matches("^ERROR: 123456\nstack traceback:\n")
end

local private_var = 'private'
public_var = 'public'

function test_variable_access(t)
    public_var2 = 'public2'

    -- Unqualified.
    t:expect(function() return UNKNOWN end)
        :label("unqualified symbol lookup"):raises("undefined symbol: UNKNOWN$")

    -- In globals.
    t:expect(_G):label("_G"):not_has('private_var')
    t:expect(_G):label("_G"):not_has('public_var')
    t:expect(_G):label("_G"):not_has('public_var2')
    t:expect(_G):label("_G"):has('error')

    -- In Lua module.
    local mod = require(module_name)
    t:expect(mod):label("mod"):not_has('private_var')
    t:expect(mod):label("mod"):has('public_var')
    t:expect(mod):label("mod"):has('public_var2')
    t:expect(mod):label("mod"):has('error')

    -- In C module.
    t:expect(mem):label("mem"):has('alloc')
    if config.HASH_SYMBOL_TABLES == 0 then
        t:expect(mem):label("mem"):not_has('UNKNOWN')
    end
end

function test_strict(t)
    local want_c = config.HASH_SYMBOL_TABLES == 0 and "undefined symbol" or nil
    for _, test in ipairs{
        {"Lua module", require(module_name), "undefined symbol"},
        {"C module", mem, want_c},
        {"C class", stderr, want_c},
    } do
        local desc, tab, want = table.unpack(test)
        if want then
            t:expect(function() return tab.UNKNOWN end)
                :label("%s attribute access", desc):raises(want)
        end
    end

end

function test_pointer(t)
    local p1 = pointer(123) + 45
    t:expect(t.expr(_G).tostring(p1)):matches('^pointer: 0?x?[0-9a-fA-F]+$')
    local p2 = p1 - 23
    t:expect(p2):label("p2"):eq(pointer(145))
    t:expect(p1 - p2):label("p1 - p2"):eq(23)

    t:expect(p1 == p1):label("p1 == p1"):eq(true)
    t:expect(p1 == p2):label("p1 == p2"):eq(false)

    t:expect(p1 < p1):label("p1 < p1"):eq(false)
    t:expect(p1 < p2):label("p1 < p2"):eq(false)
    t:expect(p2 < p1):label("p2 < p1"):eq(true)

    t:expect(p1 <= p1):label("p1 <= p1"):eq(true)
    t:expect(p1 <= p2):label("p1 <= p2"):eq(false)
    t:expect(p2 <= p1):label("p2 <= p1"):eq(true)

    t:expect(t.mexpr(getmetatable(p1)).__buffer(p1)):eq{p1}
    t:expect(t.mexpr(getmetatable(p2)).__buffer(p2)):eq{p2}
end

function test_Function_close(t)
    local called = false
    do
        local f<close> = function() called = true end
    end
    t:expect(called, "To-be-closed function wasn't called")

    local called = false
    pcall(function()
        local want = "Some error"
        local f<close> = function(err)
            called = true
            t:expect(err):label("err"):eq(want)
        end
        error(want, 0)
    end)
    t:expect(called, "To-be-closed function wasn't called on error")
end
