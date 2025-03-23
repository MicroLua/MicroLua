-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local stdio = require 'pico.stdio'
local semihosting = require 'pico.stdio.semihosting'

function test_init(t)
    semihosting.init()
    -- We can't actually test anything here, because an attached debug probe
    -- always intercepts semihosting requests, and if openocd isn't connected,
    -- they hang.
    stdio.set_driver_enabled(semihosting.driver, false)
end
