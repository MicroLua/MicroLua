-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local math = require 'math'
local io = require 'mlua.io'
local testing = require 'mlua.testing'
local pico = require 'pico'

local Test = testing.Test

function Test:_main_start()
    pico.xip_ctr(true)  -- Clear XIP counters
end

function Test:_pre_run()
    self._xip_hit, self._xip_acc = pico.xip_ctr()
end

function Test:_post_run()
    local hit, acc = pico.xip_ctr()
    self._xip_hit, self._xip_acc = hit - self._xip_hit, acc - self._xip_acc
end

-- TODO: Add threading

function Test:_print_stats(out, indent)
    if self._xip_hit then
        io.fprintf(out, "%s XIP cache: %.1f%%\n", indent,
                   (100.0 * self._xip_hit) / self._xip_acc)
    end
end

function Test:_print_main_stats()
    local hit, acc = pico.xip_ctr()
    local hr = 'N/A'
    if hit ~= 0xffffffff and acc ~= 0xffffffff then
        hit = hit + (hit >= 0 and 0.0 or 4294967296.0)
        acc = acc + (acc >= 0 and 0.0 or 4294967296.0)
        hr = ('%.1f%%'):format(100.0 * (hit / acc))
    end
    io.printf('RAM: %s bytes, binary: %s bytes, XIP cache: %s\n',
              math.tointeger(collectgarbage('count') * 1024),
              pico.flash_binary_end - pico.flash_binary_start, hr)
end
