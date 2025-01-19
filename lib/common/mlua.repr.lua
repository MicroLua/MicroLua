-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local math = require 'math'
local list = require 'mlua.list'
local string = require 'string'

-- TODO: Format numbers to full accuracy

local repr

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

local function is_list(v)
    local len = list.len(v)
    if math.type(len) ~= 'integer' or len < 0 then return false end
    local cnt = 0
    for k in pairs(v) do
        if k ~= 'n' then
            if math.type(k) ~= 'integer' or k <= 0 or k > len then
                return false
            end
            cnt = cnt + 1
        end
    end
    if len > 10 and cnt < len // 2 then return false end
    return true, len
end

local function repr_list(v, len, seen)
    local parts = list()
    for i = 1, len do parts:append(repr(v[i], seen)) end
    local vn = v.n
    if vn then parts:append(('n = %s'):format(repr(vn, seen))) end
    return ('{%s}'):format(parts:concat(', '))
end

local function repr_key(k, seen)
    if type(k) == 'string' and k:match('^[%a_][%w_]*$') then return k end
    return ('[%s]'):format(repr(k, seen))
end

local function repr_table(v, seen)
    if rawget(seen, v) then return '...' end
    rawset(seen, v, true)
    local done<close> = function() rawset(seen, v, nil) end
    local ok, len = try(is_list, v)
    if ok then return repr_list(v, len, seen) end
    local parts = list()
    for k, vk in pairs(v) do
        parts:append(('%s = %s'):format(repr_key(k, seen), repr(vk, seen)))
    end
    return ('{%s}'):format(parts:sort():concat(', '))
end

repr = function(v, seen)
    local ok, r = pcall(function() return rawget(getmetatable(v), '__repr') end)
    if ok and r then
        if seen and rawget(seen, v) then return '...' end
        return r(v, repr, seen or {})
    end
    local typ = type(v)
    if typ == 'string' then return repr_string(v)
    elseif typ == 'table' then return repr_table(v, seen or {})
    else return tostring(v) end
end

return repr
