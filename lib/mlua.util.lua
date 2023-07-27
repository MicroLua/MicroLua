-- A collection of helpers.

_ENV = mlua.Module(...)

local list = require 'mlua.list'
local string = require 'string'
local table = require 'table'

-- Identity function.
function ident(...) return ... end

-- Relational operators as functions.
function eq(a, b) return a == b end
function neq(a, b) return a ~= b end
function lt(a, b) return a < b end
function lte(a, b) return a <= b end
function gt(a, b) return a > b end
function gte(a, b) return a >= b end

local escapes = {
    [7] = '\\a', [8] = '\\b', [9] = '\\t', [10] = '\\n', [11] = '\\v',
    [12] = '\\f', [13] = '\\r', [34] = '\\"', [92] = '\\\\',
}

local function repr_string(v)
    return ('"%s"'):format(v:gsub('["\\%c]', function(c)
        local cv = c:byte(1)
        local e = escapes[cv]
        if e ~= nil then return e end
        return ('\\x%02x'):format(cv)
    end))
end

local function repr_table(v)
    local parts = list()
    for k, v in pairs(v) do
        parts:append(('[%s] = %s'):format(repr(k), repr(v)))
    end
    table.sort(parts)
    return ('{%s}'):format(table.concat(parts, ', '))
end

-- Return a string representation of the given value.
function repr(v)
    local ok, r = pcall(function() return getmetatable(v).__repr end)
    if ok and r then return r(v, repr) end
    -- TODO: Format numbers to full accuracy
    -- TODO: Detect lists and use simpler formatting
    local typ = type(v)
    if typ == 'string' then return repr_string(v)
    elseif typ == 'table' then return repr_table(v)
    else return tostring(v) end
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
        if rawget(b, k) ~= v then return false end
    end
    for k in pairs(b) do
        if rawget(a, k) == nil then return false end
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
