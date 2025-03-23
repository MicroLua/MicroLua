-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local debug = require 'debug'
local math = require 'math'
local io = require 'mlua.io'
local list = require 'mlua.list'
local util = require 'mlua.util'
local string = require 'string'
local table = require 'table'

-- TODO: Document

local raise = util.raise

-- Return a view into a slice of a list.
local function slice(list, from)
    local o = from - 1
    return setmetatable({}, {
        __index = function(_, k) return list[k + o] end,
        __len = function(_, k)
            local len = #list - o
            return len < 0 and 0 or len
        end,
    })
end

-- Parse command-line arguments into options and arguments.
function parse_args(args)
    local opts, fargs = {}, list()
    if args then
        for i, arg in ipairs(args) do
            if arg == '--' then
                fargs:len(#fargs + (#args - i))
                table.move(args, i + 1, #args, #fargs + 1, fargs)
                break
            end
            local n, v = arg:match('^%-%-([^=]+)=(.*)$')
            if not n then n, v = arg:match('^%-%-([^=]+)$'), true end
            if n then opts[n:gsub('-', '_')] = v
            else fargs:append(arg) end
        end
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
function bool_opt(default)
    return function(n, v)
        if v == nil then return default end
        if v == true or v == 'true' then return true end
        if v == false or v == 'false' then return false end
        raise("--%s: invalid argument: %s", n, v)
    end
end

-- Define a string option.
function str_opt(default)
    return function(n, v)
        if v == nil then return default end
        if type(v) == 'string' then return v end
        raise("--%s: requires a string argument", n)
    end
end

-- Define an integer option.
function int_opt(default)
    return function(n, v)
        if v == nil then return default end
        local vn = math.tointeger(tonumber(v))
        if vn then return vn end
        raise("--%s: invalid argument: %s", n, v)
    end
end

-- Define a number option.
function num_opt(default)
    return function(n, v)
        if v == nil then return default end
        local vn = tonumber(v)
        if vn then return vn end
        raise("--%s: invalid argument: %s", n, v)
    end
end

-- Parse an option according to a definition, remove it from the options and
-- return its value.
local function remove_opt(opts, name, def)
    local v = def(name:gsub('_', '-'), opts[name])
    opts[name] = nil
    return v
end

-- Run the command specified by a list of command-line arguments.
function run(argv, commands)
    local opts, args = parse_args(argv)
    local traceback = remove_opt(opts, 'traceback', bool_opt(false))
    local _, res = xpcall(function()
        local cmds = commands
        for i, cmd in ipairs(args) do
            local fn = cmds[cmd]
            if not fn then
                raise("unknown command: %s", args:concat(' ', 1, i))
            end
            if type(fn) == 'function' then
                return fn(opts, slice(args, i + 1))
            end
            cmds = fn
        end
        io.printf("Usage: %s [options] cmd [args ...]\n", argv[0])
        return 2
    end, function(err)
        if traceback and type(err) == 'string' then
            err = debug.traceback(err, 2)
        end
        return err
    end)
    return res
end
