-- A simple object model for object-oriented programming.

_ENV = require "module"(...)

-- The __call metamethod of all classes.
local function class_call(cls, ...)
    local obj = setmetatable({}, cls)
    local init = cls.__init
    if init then
        init(obj, ...)
    end
    return obj
end

-- Define a class, optionally inheriting from the given base.
function class(base)
    local cls = {__base = base}
    cls.__index = cls
    return setmetatable(cls, {__index = base, __call = class_call})
end

-- Return true iff "obj" is an instance of "cls".
function isinstance(obj, cls)
    return issubclass(getmetatable(obj), cls)
end

-- Return true iff "cls" is a subclass of "base".
function issubclass(cls, base)
    while cls do
        if cls == base then return true end
        cls = cls.__base
    end
    return false
end
