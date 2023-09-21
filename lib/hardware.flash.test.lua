_ENV = mlua.Module(...)

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
    local save = sync.save_and_disable_interrupts()
    flash.range_erase(offs, flash.SECTOR_SIZE)
    sync.restore_interrupts(save)
    t:expect(t:expr(mem).read(addressmap.XIP_BASE + offs, flash.SECTOR_SIZE))
        :eq(('\xff'):rep(flash.SECTOR_SIZE))

    -- Program and verify the first page of the last sector.
    local data = ('abcdefghijklmnopqrstuvwxyz012345'):rep(flash.PAGE_SIZE // 32)
    local save = sync.save_and_disable_interrupts()
    flash.range_program(offs, data)
    sync.restore_interrupts(save)
    t:expect(t:expr(mem).read(addressmap.XIP_BASE + offs, #data)):eq(data)
end

function test_get_unique_id(t)
    local save = sync.save_and_disable_interrupts()
    local id = flash.get_unique_id()
    sync.restore_interrupts(save)
    t:expect(id):label("get_unique_id()"):eq(unique_id.get_unique_board_id())
end

function test_do_cmd(t)
    -- Issue a RUID command.
    local save = sync.save_and_disable_interrupts()
    local rx = flash.do_cmd('\x4b1234' .. ('_'):rep(flash.UNIQUE_ID_SIZE_BYTES))
    sync.restore_interrupts(save)
    t:expect(rx:sub(6)):label("do_cmd()"):eq(unique_id.get_unique_board_id())
end
