-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local io = require 'mlua.io'
local mem = require 'mlua.mem'
local testing = require 'mlua.testing'
local pico = require 'pico'
local standard_link = require 'pico.standard_link'

local Test = testing.Test

local function xip_hit_rate(hit1, acc1, hit2, acc2)
    if hit2 == 0xffffffff or acc2 == 0xffffffff then return 'N/A' end
    local hit, acc = hit2 - hit1, acc2 - acc1
    hit = hit + (hit >= 0 and 0.0 or 4294967296.0)
    acc = acc + (acc >= 0 and 0.0 or 4294967296.0)
    return ('%.1f%%'):format(100.0 * (hit / acc))
end

local pre_run = Test._pre_run
function Test:_pre_run()
    if not self._parent then pico.xip_ctr(true) end  -- Clear XIP counters
    pre_run(self)
    self._xip_hit, self._xip_acc = pico.xip_ctr()
end

local post_run = Test._post_run
function Test:_post_run()
    self._xip_hit = xip_hit_rate(self._xip_hit, self._xip_acc, pico.xip_ctr())
    self._xip_acc = nil
    post_run(self)
end

local print_stats = Test._print_stats
function Test:_print_stats(out)
    print_stats(self, out)
    if self._parent then
        io.fprintf(out, "Flash: XIP cache: %s\n", self._xip_hit)
    else
        local xhr = xip_hit_rate(0, 0, pico.xip_ctr())
        local size = standard_link.heap_end - standard_link.heap_start
        local alloc, used = mem.mallinfo()
        io.fprintf(
            out, "Heap: %s B, allocated: %s B (%.1f%%), used: %s B (%.1f%%)\n",
            size, alloc, 100 * (alloc / size), used, 100 * (used / size))
        io.fprintf(out, "Flash: binary: %s B, XIP cache: %s\n",
                   pico.flash_binary_end - pico.flash_binary_start, xhr)
    end
end
