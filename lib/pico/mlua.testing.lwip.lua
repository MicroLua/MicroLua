-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local config = require 'mlua.config'
local io = require 'mlua.io'
local mem = require 'mlua.mem'
local oo = require 'mlua.oo'
local time = require 'mlua.time'
local lwip = require 'pico.lwip'
local dns = require 'pico.lwip.dns'
local tcp = require 'pico.lwip.tcp'
local string = require 'string'

Set = oo.class('Set')

function Set:__init() self._len = 0 end
function Set:__len() return self._len end

function Set:add(v)
    if not rawget(self, v) then
        rawset(self, v, true)
        self._len = self._len + 1
    end
    return self._len
end

local function prng(seed)
    seed = (1103515245 * seed + 12345) & 0x7fffffff
    return seed, (seed >> 23) & 0xff
end

function generate_data(pid, size, buf, off)
    local seed, v = size + pid
    mem.set(buf, off, pid & 0xff, (pid >> 8) & 0xff)
    for o = off + 2, off + size - 1 do
        seed, v = prng(seed)
        mem.set(buf, o, v)
    end
end

function verify_data(buf, off, size)
    local pid, pidh = mem.get(buf, off, 2)
    pid = pid | (pidh << 8)
    local seed, v = size + pid
    for o = off + 2, off + size - 1 do
        local d = mem.get(buf, o)
        seed, v = prng(seed)
        if d ~= v then return pid, false end
    end
    return pid, true
end

function connect_control(t)
    -- TODO: Support IPv6
    local dl = time.deadline(5 * time.sec)
    local addr = lwip.assert(dns.gethostbyname(config.SERVER_ADDR, nil, dl))
    local sock = lwip.assert(tcp.new())
    t:cleanup(function() sock:close() end)
    lwip.assert(sock:connect(addr, config.SERVER_PORT, dl))
    local line = lwip.assert(io.read_line(sock, dl))
    local port = line:match('^PORT ([0-9]+)\n')
    t:assert(port, "Unexpected PORT line: @{+WHITE}%s@{NORM}", t:repr(line))
    return sock, addr, tonumber(port)
end
