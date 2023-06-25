-- A collection of helpers.

_ENV = mlua.Module(...)

local table = require 'table'

-- TODO: Refactor the functionality below as iterators

-- Return all arguments.
function ident(...) return ... end

-- Return true iff a == b.
function eq(a, b) return a == b end

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
    local res, i = {}, 1
    for key, value in pairs(tab) do
        if not filter or filter(key, value) then
            res[i] = key
            i = i + 1
        end
    end
    return res
end

-- Return the values of the given table, optionally filtered.
function values(tab, filter)
    local res, i = {}, 1
    for key, value in pairs(tab) do
        if not filter or filter(key, value) then
            res[i] = value
            i = i + 1
        end
    end
    return res
end

-- Sort the given list and return it.
function sort(list, comp)
    table.sort(list, comp)
    return list
end
