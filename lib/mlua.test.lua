_ENV = mlua.Module(...)

local function has(tab, name)
    local ok, value = pcall(function() return tab[name] end)
    return ok and value ~= nil
end

local private_var = 'private'
public_var = 'public'

function test_variable_access(t)
    public_var2 = 'public2'
    t:expect(not has(_G, 'private_var'), "private_var is in _G")
    t:expect(not has(_G, 'public_var'), "public_var is in _G")
    t:expect(not has(_G, 'public_var2'), "public_var2 is in _G")
    t:expect(has(_G, 'error'), "error is not in _G")

    local mod = require 'mlua.test'
    local got = tostring(mod)
    t:expect(got:find('^mlua%.Module:'), "Unexpected class name: %q", got)
    t:expect(not has(mod, 'private_var'), "private_var is in module")
    t:expect(has(mod, 'public_var'), "public_var is not in module")
    t:expect(has(mod, 'public_var2'), "public_var2 is not in module")
    t:expect(has(mod, 'error'), "error is not in module")
end

function test_strict_mode(t)
    local ok, err = pcall(function() return unknown_symbol end)
    t:assert(not ok, "Unknown symbol lookup succeeded")
    t:expect(err:find("undefined symbol: unknown_symbol$"),
             "Unexpected error: %q", err)
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
