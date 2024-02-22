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
