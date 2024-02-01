-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local config = require 'mlua.config'
local pico = require 'pico'
local board = require 'pico.board'

function test_symbols(t)
    t:expect(t:expr(board).PICO_FLASH_SPI_CLKDIV):eq(pico.FLASH_SPI_CLKDIV)
    t:expect(t:expr(board).PICO_FLASH_SIZE_BYTES):eq(pico.FLASH_SIZE_BYTES)
end

function test_DEFAULT_symbols(t)
    if config.HASH_SYMBOL_TABLES ~= 0 then t:skip("Hashed symbol tables") end
    for k, v in pairs(pico) do
        if k:match('DEFAULT_.*') and v then
            t:expect(t:expr(board)['PICO_' .. k]):eq(v)
        end
    end
end
