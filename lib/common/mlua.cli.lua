-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local list = require 'mlua.list'
local util = require 'mlua.util'
local string = require 'string'
local table = require 'table'

-- TODO: Document

local raise = util.raise

-- Parse command-line arguments into options and arguments.
function parse_args(args)
    local opts, fargs = {}, list()
    for i, arg in ipairs(args) do
        if arg == '--' then
            fargs:set_len(#fargs + (#args - i))
            table.move(args, i + 1, #args, #fargs + 1, fargs)
            break
        end
        local n, v = arg:match('^%-%-([^=]+)=(.*)$')
        if not n then n, v = arg:match('^%-%-([^=]+)$'), true end
        if n then opts[n:gsub('-', '_')] = v
        else fargs:append(arg) end
    end
    return opts, fargs
end

-- Parse options according to option definitions.
function parse_opts(opts, defs)
    for k, v in pairs(opts) do
        local def, n = defs[k], k:gsub('_', '-')
        if not def then raise("unknown option: --%s", n) end
        opts[k] = def(n, v)
    end
    for k, def in pairs(defs) do
        if not opts[k] then opts[k] = def(k:gsub('_', '-'), nil) end
    end
end

-- Define a boolean option.
function bool_opt(def)
    return function(n, v)
        if v == nil then return def end
        if v == true or v == 'true' then return true end
        if v == false or v == 'false' then return false end
        raise("--%s: invalid argument: %s", n, v)
    end
end

-- Define a string option.
function str_opt(def)
    return function(n, v)
        if v == nil then return def end
        if type(v) == 'string' then return v end
        raise("--%s: requires a string argument", n)
    end
end

-- Define a number option.
function num_opt(def)
    return function(n, v)
        if v == nil then return def end
        local i = tonumber(v)
        if i then return i end
        raise("--%s: invalid argument: %s", n, v)
    end
end

