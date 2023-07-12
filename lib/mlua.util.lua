-- A collection of helpers.

_ENV = mlua.Module(...)

local string = require 'string'
local table = require 'table'

-- Return all arguments.
function ident(...) return ... end

-- Return true iff a == b.
function eq(a, b) return a == b end

-- Return a string representation of the given value.
function str(v)
    local typ = type(v)
    if typ == 'string' then return ('%q'):format(v)
    elseif typ ~= 'table' then return tostring(v) end
    local parts = {}
    for k, v in pairs(v) do
        append(parts, ('[%s] = %s'):format(str(k), str(v)))
    end
    table.sort(parts)
    return '{' .. table.concat(parts, ', ') .. '}'
end

-- Return an object whose __tostring metamethod calls a function (str by
-- default) on its argument.
function lstr(v, fn)
    return setmetatable({}, {__tostring = function() return (fn or str)(v) end})
end

-- Return the length of a list.
function len(list) return not list and 0 or list[0] or #list end

-- Append an element to a list, and return the updated list.
function append(list, el)
    if not list then list = {[0] = 0} end
    local len = (list[0] or #list) + 1
    list[len] = el
    list[0] = len
    return list
end

-- Return the keys of the given table, optionally filtered.
function keys(tab, filter)
    local res, len = {}, 0
    for key, value in pairs(tab) do
        if not filter or filter(key, value) then
            len = len + 1
            res[len] = key
        end
    end
    res[0] = len
    return res
end

-- Return the values of the given table, optionally filtered.
function values(tab, filter)
    local res, len = {}, 0
    for key, value in pairs(tab) do
        if not filter or filter(key, value) then
            len = len + 1
            res[len] = value
        end
    end
    res[0] = len
    return res
end

-- Sort the given list and return it.
function sort(list, comp)
    table.sort(list, comp)
    return list
end

-- Return true iff the key / value pairs of the given tables compare equal.
function table_eq(a, b)
    for k, v in pairs(a) do
        if b[k] ~= v then return false end
    end
    for k in pairs(b) do
        if a[k] == nil then return false end
    end
    return true
end

-- Return a table comparator comparing table pairs by the elements at the given
-- keys.
function table_comp(keys)
    return function(a, b)
        for _, k in ipairs(keys) do
            local ak, bk = a[k], b[k]
            if ak < bk then return true
            elseif ak > bk then return false end
        end
        return false
    end
end
