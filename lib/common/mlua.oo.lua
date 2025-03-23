-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

-- A simple object model for object-oriented programming.
--
-- Classes are created with class, providing a name and optionally a base class.
-- Instances are created by calling the class, optionally with arguments. The
-- latter are passed to the instance initializer __init, if it is defined.
--
-- Metamethods can be defined on classes, and have the expected effect. They are
-- copied from the base class to the subclass at class creation time. This is
-- necessary because Lua gets metamethods using a raw access.

-- The class of all classes.
local Metaclass = {}

function Metaclass.__tostring(cls) return 'class ' .. cls.__name end

function Metaclass.__call(cls, ...)
    local obj = setmetatable({}, cls)
    local init = cls.__init
    if init then init(obj, ...) end
    return obj
end

-- Define a class, optionally inheriting from the given base class.
function class(name, base)
    local cls = {}
    if base then
        for k, v in pairs(base) do
            if k:sub(1, 2) == '__' then cls[k] = v end
        end
    end
    cls.__name, cls.__base, cls.__index = name, base, cls
    return setmetatable(cls, not base and Metaclass or {
        __index = cls.__base,
        __tostring = Metaclass.__tostring,
        __call = Metaclass.__call,
    })
end

-- Return true iff "cls" is a subclass of "base".
function issubclass(cls, base)
    while cls do
        if cls == base then return true end
        cls = cls.__base
    end
    return false
end

-- Return true iff "obj" is an instance of "cls".
function isinstance(obj, cls) return issubclass(getmetatable(obj), cls) end
