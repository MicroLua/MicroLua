-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local flash = require 'hardware.flash'
local addressmap = require 'hardware.regs.addressmap'
local sync = require 'hardware.sync'
local mem = require 'mlua.mem'
local pico = require 'pico'
local unique_id = require 'pico.unique_id'
local string = require 'string'

function test_erase_program(t)
    local offs = (pico.FLASH_SIZE_BYTES - flash.SECTOR_SIZE)
                 & ~(flash.SECTOR_SIZE - 1)
    if offs < pico.flash_binary_end - pico.flash_binary_start then
        t:skip("no free sector in flash memory")
    end

    -- Erase the last sector in flash.
    do
        local save = sync.save_and_disable_interrupts()
        local done<close> = function() sync.restore_interrupts(save) end
        flash.range_erase(offs, flash.SECTOR_SIZE)
    end
    t:expect(t.expr(mem).read(addressmap.XIP_BASE, offs, flash.SECTOR_SIZE))
        :eq(('\xff'):rep(flash.SECTOR_SIZE))

    -- Program and verify the first page of the last sector.
    local data = ('abcdefghijklmnopqrstuvwxyz012345'):rep(flash.PAGE_SIZE // 32)
    do
        local save = sync.save_and_disable_interrupts()
        local done<close> = function() sync.restore_interrupts(save) end
        flash.range_program(offs, data)
    end
    t:expect(t.expr(mem).read(addressmap.XIP_BASE, offs, #data)):eq(data)
end

function test_get_unique_id(t)
    local id
    do
        local save = sync.save_and_disable_interrupts()
        local done<close> = function() sync.restore_interrupts(save) end
        id = flash.get_unique_id()
    end
    t:expect(id):label("get_unique_id()"):eq(unique_id.get_unique_board_id())
end

function test_do_cmd(t)
    -- Issue a RUID command.
    local rx
    do
        local save = sync.save_and_disable_interrupts()
        local done<close> = function() sync.restore_interrupts(save) end
        rx = flash.do_cmd('\x4b1234' .. ('_'):rep(flash.UNIQUE_ID_SIZE_BYTES))
    end
    t:expect(rx:sub(6)):label("do_cmd()"):eq(unique_id.get_unique_board_id())
end
