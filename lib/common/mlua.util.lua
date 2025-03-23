-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local math = require 'math'
local list = require 'mlua.list'
local string = require 'string'

-- Identity function.
function ident(...) return ... end

-- Relational operators as functions.
function eq(a, b) return a == b end
function neq(a, b) return a ~= b end
function lt(a, b) return a < b end
function lte(a, b) return a <= b end
function gt(a, b) return a > b end
function gte(a, b) return a >= b end

-- Return the value associated with the given key, or nil if the lookup fails.
function get(tab, key)
    local ok, v = pcall(function() return tab[key] end)
    if ok then return v end
end

-- Raise an error without location information.
function raise(format, ...) return error(format:format(...), 0) end

-- Raise an error if the first argument is false, otherwise return all
-- arguments.
function check(...)
    local ok, msg = ...
    if not ok then return error(msg, 0) end
    return ...
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
    res.n = len
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
    res.n = len
    return res
end

-- Return true iff the (key, value) pairs of the given tables compare equal.
function table_eq(a, b)
    if a == nil or b == nil then return rawequal(a, b) end
    for k, v in pairs(a) do
        if rawget(b, k) ~= v then return false end
    end
    for k in pairs(b) do
        if rawget(a, k) == nil then return false end
    end
    return true
end

-- Return a shallow copy of the given table.
function table_copy(tab)
    if not tab then return end
    local res = {}
    for k, v in pairs(tab) do rawset(res, k, v) end
    return setmetatable(res, getmetatable(tab))
end

-- Return a comparison function comparing table pairs by the elements at the
-- given keys.
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

-- Compute the "p"th percentile of a sorted list of values.
function percentile(values, p)
    local rank = #values * (0.01 * p) + 0.5
    if rank < 1 then rank = 1.0
    elseif rank > #values then rank = 1.0 * #values end
    local idx = math.tointeger(math.floor(rank))
    local perc = 1.0 * values[idx]
    local rem = rank - idx
    if rem > 0 then perc = perc + (values[idx + 1] - perc) * rem end
    return perc
end
