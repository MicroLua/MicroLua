-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local pico = require 'pico'

function set_up(t)
    t:printf("SDK: %s, board: %s, build type: %s, build target: %s\n",
             pico.SDK_VERSION_STRING, pico.board, pico.build_type,
             pico.build_target)
    t:printf("Flash: %08x, binary: %08x - %08x (%.1f%%)\n",
             pico.FLASH_SIZE_BYTES,
             pico.flash_binary_start - pointer(0),
             pico.flash_binary_end - pointer(0),
             (pico.flash_binary_end - pico.flash_binary_start)
             / pico.FLASH_SIZE_BYTES * 100)
end

function test_flash(t)
    t:expect(pico.flash_binary_start <= pico.flash_binary_end,
             "flash_binary_start > flash_binary_end")
end

function test_error_str(t)
    t:expect(t.expr(pico).error_str(pico.ERROR_TIMEOUT)):eq("timeout")
end
