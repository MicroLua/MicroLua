-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local platform = require 'mlua.platform'

function set_up(t)
    t:printf("Platform: %s\n", platform.name)
end

function test_flash(t)
    local flash = platform.flash
    if not flash then t:skip("platform has no flash") end
    t:expect(flash):label("flash")
        :has('ptr'):has('size'):has('write_size'):has('erase_size')
end
