_ENV = mlua.Module(...)

local io = require 'mlua.io'
local pico = require 'pico'
local table = require 'table'

local module_name = ...

local function has(tab, name)
    local ok, value = pcall(function() return tab[name] end)
    return ok and value ~= nil
end

local private_var = 'private'
public_var = 'public'

function test_variable_access(t)
    public_var2 = 'public2'

    -- Unqualified.
    local ok, err = pcall(function() return UNKNOWN end)
    t:assert(not ok, "Lookup succeeded, got: %s", err)
    t:expect(err:find("undefined symbol: UNKNOWN$"),
             "Unexpected error: %q", err)

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
    for _, test in ipairs{
        {"Lua module", require(module_name)},
        {"C module", pico},
        {"C class", stderr},
    } do
        local desc, tab = table.unpack(test)
        local ok, err = pcall(function() return tab.UNKNOWN end)
        t:assert(not ok, "%s: Lookup succeeded, got: %s", desc, err)
        t:expect(err:find("undefined symbol: UNKNOWN$"),
                 "Unexpected error: %q", err)
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

function test_print(t)
    local b = io.Buffer()
    t:patch(_G, 'stdout', b)

    local v = setmetatable({}, {__tostring = function() return '(v)' end})
    print(1, 2.3, "4-5", v)
    print(6, 7, 8)
    t:expect(tostring(b)):label('output')
        :eq("1\t" .. tostring(2.3) .."\t4-5\t(v)\n6\t7\t8\n")
end
