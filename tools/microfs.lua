-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local io = require 'io'
local lfs = require 'microfs.lfs'
local string = require 'string'

-- Raise an error without position information.
local function raise(format, ...)
    return error(format:format(...), 0)
end

-- Raise an error if the first argument is false, otherwise return all
-- arguments.
local function assert(...)
    local ok, msg = ...
    if not ok then return error(msg, 0) end
    return ...
end

-- Print to stdout.
local function printf(format, ...) io.stdout:write(format:format(...)) end

-- Return a table that calls the given function when it is closed.
local function defer(fn) return setmetatable({}, {__close = fn}) end

-- Read a file and return its content.
local function read_file(path)
    local f<close> = assert(io.open(path, 'r'))
    return f:read('a')
end

function main(...)
    local fs<close> = lfs.new(1 << 20)
    local data = fs:unmount()
    printf("Hello, world!\n")
    for _, arg in ipairs{...} do printf("arg: %s\n", arg) end
end
