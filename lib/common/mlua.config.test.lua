-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local config = require 'mlua.config'

function test_name(t)
    t:expect(t.expr(config).a):eq(1)
    t:expect(t.expr(config).b):eq("test")
end
