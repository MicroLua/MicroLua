-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local base = require 'hardware.base'
local addressmap = require 'hardware.regs.addressmap'
local watchdog = require 'hardware.regs.watchdog'
local list = require 'mlua.list'

local scratch = addressmap.WATCHDOG_BASE + watchdog.SCRATCH0_OFFSET

function test_read32_write32(t)
    for _, test in ipairs{
        {1, 2, 3},
        {0x01234567, 0x89abcdef, 0xfdb97531, 0xeca86420},
        {0, 0, 0, 0},
    } do
        base.write32(scratch, list.unpack(test))
        local got = list.pack(base.read32(scratch, #test))
        t:expect(got):label("read32"):eq(test)
    end
end

function test_read_write_width(t)
    base.write32(scratch, 0x01234567)
    t:expect(t.expr(base).read8(scratch)):eq(0x67)
    t:expect(t.expr(base).read8(scratch + 1)):eq(0x45)
    t:expect(t.expr(base).read8(scratch + 2)):eq(0x23)
    t:expect(t.expr(base).read8(scratch + 3)):eq(0x01)
    t:expect(t.expr(base).read16(scratch)):eq(0x4567)
    t:expect(t.expr(base).read16(scratch + 2)):eq(0x0123)
    t:expect(t.expr(base).read32(scratch)):eq(0x01234567)

    -- Narrow writes to hardware registers are replicated across the whole word.
    base.write8(scratch, 0x42)
    t:expect(t.expr(base).read32(scratch)):eq(0x42424242)
    base.write16(scratch, 0xabcd)
    t:expect(t.expr(base).read32(scratch)):eq(0xabcdabcd)
end

function test_alignment(t)
    t:expect(t.expr(base).read16(scratch + 1)):raises("not 16%-bit aligned")
    t:expect(t.expr(base).read32(scratch + 2)):raises("not 32%-bit aligned")
end

function test_atomic_hw_bit_ops(t)
    base.write32(scratch, 0x01234567)
    base.hw_set_bits(scratch, 0x12345678)
    t:expect(base.read32(scratch)):label("set"):eq(0x1337577f)
    base.hw_clear_bits(scratch, 0xe1241546)
    t:expect(base.read32(scratch)):label("clear"):eq(0x12134239)
    base.hw_xor_bits(scratch, 0x8421c639)
    t:expect(base.read32(scratch)):label("xor"):eq(0x96328400)
    base.hw_write_masked(scratch, 0x01234567, 0x00ff0000)
    t:expect(base.read32(scratch)):label("write masked"):eq(0x96238400)
    base.hw_write_masked(scratch, 0x01234567, 0xf000f0f0)
    t:expect(base.read32(scratch)):label("write masked"):eq(0x06234460)
end
