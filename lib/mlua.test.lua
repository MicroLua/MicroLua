_ENV = mlua.Module(...)

local config = require 'mlua.config'
local io = require 'mlua.io'
local pico = require 'pico'
local table = require 'table'

local module_name = ...

function set_up(t)
    t:printf("Lua: %s\n", _RELEASE:gsub('[^ ]+ (.*)', '%1'))
end

local function has(tab, name)
    local ok, value = pcall(function() return tab[name] end)
    return ok and value ~= nil
end

local private_var = 'private'
public_var = 'public'

function test_variable_access(t)
    public_var2 = 'public2'

    -- Unqualified.
    t:expect(function() return UNKNOWN end)
        :label("unqualified symbol lookup"):raises("undefined symbol: UNKNOWN$")

    -- In globals.
    t:expect(not has(_G, 'private_var'), "private_var is in _G")
    t:expect(not has(_G, 'public_var'), "public_var is in _G")
    t:expect(not has(_G, 'public_var2'), "public_var2 is in _G")
    t:expect(has(_G, 'error'), "error is not in _G")

    -- In Lua module.
    local mod = require(module_name)
    t:expect(not has(mod, 'private_var'), "private_var is in module")
    t:expect(has(mod, 'public_var'), "public_var is not in module")
    t:expect(has(mod, 'public_var2'), "public_var2 is not in module")
    t:expect(has(mod, 'error'), "error is not in module")

    -- In C module.
    t:expect(has(pico, 'OK'), "OK is not in module")
    t:expect(not has(pico, 'UNKNOWN'), "UNKNOWN is in module")
end

function test_strict(t)
    local want_c = config.HASH_SYMBOL_TABLES == 0 and "undefined symbol" or nil
    for _, test in ipairs{
        {"Lua module", require(module_name), "undefined symbol"},
        {"C module", pico, want_c},
        {"C class", stderr, "undefined symbol"},
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
