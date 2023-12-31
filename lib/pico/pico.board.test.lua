-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local pico = require 'pico'
local board = require 'pico.board'

function test_DEFAULT_symbols(t)
    -- TODO: This doesn't test anything with read-only tables
    for k, v in pairs(pico) do
        if k:match('DEFAULT_.*') and v then
            t:expect(t:expr(board)['PICO_' .. k]):eq(v)
        end
    end
end
