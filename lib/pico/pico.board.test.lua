-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local pico = require 'pico'
local board = require 'pico.board'

function test_symbols(t)
    t:expect(t.expr(board).PICO_FLASH_SPI_CLKDIV):eq(pico.FLASH_SPI_CLKDIV)
    t:expect(t.expr(board).PICO_FLASH_SIZE_BYTES):eq(pico.FLASH_SIZE_BYTES)
end
