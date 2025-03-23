-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local lwip = require 'lwip'
local dns = require 'lwip.dns'
local stats = require 'lwip.stats'
local tcp = require 'lwip.tcp'
local config = require 'mlua.config'
local io = require 'mlua.io'
local mem = require 'mlua.mem'
local oo = require 'mlua.oo'
local testing = require 'mlua.testing'
local time = require 'mlua.time'
local string = require 'string'
local table = require 'table'

Set = oo.class('Set')

function Set:__init() self._len = 0 end
function Set:__len() return self._len end

function Set:add(v)
    if v and not rawget(self, v) then
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

IPV4 = dns.ADDRTYPE_IPV4
IPV6 = dns.ADDRTYPE_IPV6

Control = oo.class('Control')

function Control:__init(t, atype)
    local dl = time.deadline(5 * time.sec)
    self.addr = lwip.assert(
        dns.gethostbyname(config.SERVER_ADDR, atype or IPV4, dl))
    self.sock = lwip.assert(tcp.new())
    t:cleanup(function() self.sock:close() end)
    lwip.assert(self.sock:connect(self.addr, config.SERVER_PORT, dl))
    local line = self:recv(dl)
    local port = line:match('^PORT ([0-9]+)\n$')
    t:assert(port, "Unexpected PORT line: @{+WHITE}%s@{NORM}", t:repr(line))
    self.port = tonumber(port)
end

function Control:send(dl, fmt, ...)
    return lwip.assert(self.sock:send(fmt:format(...), dl))
end

function Control:recv(dl)
    return lwip.assert(io.read_line(self.sock, dl))
end

if not testing.overrides[...] then
    testing.overrides[...] = true
    local Test = testing.Test

    local pre_run = Test._pre_run
    function Test:_pre_run()
        pre_run(self)
        if self._parent then return end
        stats.mem_reset()
        for i = 0, stats.MEMP_COUNT - 1 do stats.memp_reset(i) end
    end

    local print_stats = Test._print_stats
    function Test:_print_stats(out)
        print_stats(self, out)
        if stats.mem then
            local st = stats.mem()
            io.fprintf(
                out, "lwIP mem: avail: %s B, used: %s B, max: %s B\n",
                st.avail, st.used, st.max)
        end
        if stats.memp then
            for i = 0, stats.MEMP_COUNT - 1 do
                local st, name = stats.memp(i)
                io.fprintf(
                    out, "lwIP memp[%s]: avail: %s, used: %s, max: %s\n",
                    name or i, st.avail, st.used, st.max)
            end
        end
    end
end
