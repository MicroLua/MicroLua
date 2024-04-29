-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

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
        :label("failure"):eq{nil, "boom"}
    t:expect(t.mexpr(_G).try(function() thread.yield() return 1, 2 end))
        :label("yielding"):eq{1, 2}
end

function test_equal(t)
    local tab = setmetatable({}, {__eq = function() return true end})
    t:expect(t.expr(_G).equal(tab, 1)):eq(true)
    t:expect(t.expr(_G).equal(1, tab)):eq(true)
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
            t:expect(err == want,
                     "Unexpected error: got %q, want %q", err, want)
        end
        error(want, 0)
    end)
    t:expect(called, "To-be-closed function wasn't called on error")
end
