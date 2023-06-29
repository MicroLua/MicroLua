-- Base functionality.

local package = require 'package'

-- Forward key lookups in the module to the global environment.
local mt = {__name = 'mlua.Module', __index = _G}

-- Create a new module, register it and return it.
--
-- To make a .lua file a module, add the following as its first line:
--
--   _ENV = mlua.Module(...)
--
-- This creates a new module, registers it with the name given to the require
-- call, and assigns it to the block's environment. The block can then define
-- the module contents.
--
--  - Local variables and functions are private to the module.
--  - Non-local variables and functions are exported.
--  - Lookups of undefined symbols are forwarded to the global environment.
--  - Lookups of undefined symbols in the global environment raise an error.
local function Module(name)
    local m = setmetatable({}, mt)
    package.loaded[name] = m
    return m
end

_ENV = Module(...)
_ENV.Module = Module
