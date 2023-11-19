-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = mlua.module(...)

local clocks = require 'hardware.clocks'
local math = require 'math'
local platform = require 'pico.platform'
local time = require 'pico.time'

function set_up(t)
    t:printf("Chip version: %d, ROM version: %d\n",
             platform.rp2040_chip_version(), platform.rp2040_rom_version())
end

function test_busy_wait_at_least_cycles(t)
    local delay = 10000
    local cycles = math.floor(delay * (clocks.get_hz(clocks.clk_sys) / 1e6))
    local start = time.get_absolute_time()
    platform.busy_wait_at_least_cycles(cycles)
    local now = time.get_absolute_time()
    local want = start + delay
    t:expect(want <= now, "Short wait, by: %s us", want - now)
    t:expect(now < want + 200, "Long wait, delay: %s us", now - want)
end

function test_get_core_num(t)
    local got = platform.get_core_num()
    t:expect(got == 0, "get_core_num() = %s, want 0", got)
end
