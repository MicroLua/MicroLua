-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local stdio = require 'pico.stdio'
local usb = require 'pico.stdio.usb'

function test_enable(t)
    t:expect(type(usb.connected())):label("type(connected())"):eq('boolean')
    local done<close> = function()
        stdio.set_driver_enabled(usb.driver, true)
    end
    stdio.set_driver_enabled(usb.driver, false)
end
