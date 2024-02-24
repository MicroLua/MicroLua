-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local pio = require 'hardware.pio'
local addressmap = require 'hardware.regs.addressmap'

function test_config(t)
    local cfg = pio.get_default_sm_config()
        :set_out_pins(1, 2)
        :set_set_pins(3, 1)
        :set_clkdiv_int_frac(250)
    -- TODO
end

function test_PIO_sm(t)
    local inst = pio[0]
    for i = 0, 3 do
        for j = 0, 3 do
            t:expect(inst:sm(i) == inst:sm(j))
                :label("inst:sm(%s) == inst:sm(%s)", i, j):eq(i == j)
        end
    end
end

function test_PIO_index_base(t)
    for i = 0, pio.NUM - 1 do
        local inst = pio[i]
        t:expect(t:expr(inst):get_index()):eq(i)
        t:expect(t:expr(inst):regs_base())
            :eq(addressmap[('PIO%s_BASE'):format(i)])
    end
end

local prog = {
                -- loop:
    0xa0c1,     -- 0:       mov(isr, x)
    0x8020,     -- 1:       push()
    0x0040,     -- 2:       jmp(x_dec, loop)
                -- start:
    0x80a0,     -- 3:       pull()
    0xa027,     -- 4:       mov(x, osr)
    0x0040,     -- 5:       jmp(x_dec, loop)
    labels = {start = 3},
}

function test_program(t)
    -- Load the program.
    local inst = pio[0]
    t:assert(t:expr(inst):can_add_program(prog)):eq(true)
    local off = inst:add_program(prog)
    t:expect(off):label("offset"):eq(32 - #prog)
    t:cleanup(function() inst:remove_program(prog, off) end)

    -- Claim a state machine.
    local smi = inst:claim_unused_sm()
    t:cleanup(function() inst:unclaim(smi) end)
    local sm = inst:sm(smi)
    t:expect(t:expr(sm):index()):eq(smi)
    t:expect(t:expr(sm):is_claimed()):eq(true)

    -- Configure and start the state machine.
    local cfg = pio.get_default_sm_config()
    cfg:set_wrap(prog.labels.start + off, #prog + off)
    sm:init(prog.labels.start + off, cfg)
    t:cleanup(function() sm:set_enabled(false) end)
    sm:set_enabled(true)

    -- Exercise the program.
    for _, n in ipairs{2, 3, 5, 7, 11} do
        t:context{n = n}
        sm:put_blocking(n)
        local i = n
        while i > 0 do
            i = i - 1
            t:expect(t:expr(sm):get_blocking()):eq(i)
        end
    end
end
