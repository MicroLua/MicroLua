-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local errors = require 'mlua.errors'

function test_codes(t)
    t:expect(errors.EOK):eq(0)
    t:expect(errors.EINVAL):lt(0)
    t:expect(errors.ENOENT):lt(0)
    t:expect(errors.EINVAL):neq(errors.ENOENT)
end

function test_message(t)
    t:expect(t.expr(errors).message(errors.EINVAL)):eq("invalid argument")
    t:expect(t.expr(errors).message(errors.ENOENT))
        :eq("no such file or directory")
end
