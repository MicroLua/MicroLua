_ENV = require 'mlua.module'(...)

local function has(tab, name)
    local res, value = pcall(function() return tab[name] end)
    return res and value ~= nil
end

local private_var = 'private'
public_var = 'public'

function test_variable_access(t)
    public_var2 = 'public2'
    t:expect(not has(_G, 'private_var'), "private_var is in _G")
    t:expect(not has(_G, 'public_var'), "public_var is in _G")
    t:expect(not has(_G, 'public_var2'), "public_var2 is in _G")
    t:expect(has(_G, 'error'), "error is not in _G")
    local mod = require 'mlua.module.test'
    t:expect(not has(mod, 'private_var'), "private_var is in module")
    t:expect(has(mod, 'public_var'), "public_var is not in module")
    t:expect(has(mod, 'public_var2'), "public_var2 is not in module")
    t:expect(has(mod, 'error'), "error is not in module")
end

function test_strict_mode(t)
    local res, err = pcall(function() return unknown_symbol end)
    t:assert(not res, "Unknown symbol lookup succeeded")
    t:expect(err:find("Undefined symbol: unknown_symbol$"),
             "Unexpected error: %q", err)
end
