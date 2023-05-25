-- A module creation package.
--
-- To make a .lua file a module, add the following as its first line:
--
--   _ENV = require 'mlua.module'(...)
--
-- This creates a new module, registers it with the name given to the require
-- call, and assigns it to the block's environment. The block can then define
-- the module contents.
--
--  - Local variables and functions are private to the module.
--  - Non-local variables and functions are exported.
--  - Lookups of undefined symbols are forwarded to the global environment.
--  - Lookups of undefined symbols in the global environment raise an error.

local package = require 'package'

-- Make the lookup of undefined keys in the global environment an error.
local gmt = {}
function gmt:__index(name) error("Undefined symbol: " .. name, 2) end
setmetatable(_G, gmt)

-- Forward key lookups in the module to the global environment.
local mt = {__index = _G}

-- Create a new module, register it and return it.
local function new(name)
    local m = {}
    setmetatable(m, mt)
    package.loaded[name] = m
    return m
end

-- Return the module factory as the module's content, so that it can be called
-- directly on the "require" line.
return new
