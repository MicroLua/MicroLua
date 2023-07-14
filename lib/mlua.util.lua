-- A collection of helpers.

_ENV = mlua.Module(...)

local list = require 'mlua.list'
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
    local parts = list()
    for k, v in pairs(v) do
        parts:append(('[%s] = %s'):format(str(k), str(v)))
    end
    table.sort(parts)
    return '{' .. table.concat(parts, ', ') .. '}'
end

-- Return the keys of the given table, optionally filtered.
function keys(tab, filter)
    local res, len = list(), 0
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
    local res, len = list(), 0
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
function sort(items, comp)
    table.sort(items, comp)
    return items
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
        for _, k in list.ipairs(keys) do
            local ak, bk = a[k], b[k]
            if ak < bk then return true
            elseif ak > bk then return false end
        end
        return false
    end
end
