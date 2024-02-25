-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local pio = require 'hardware.pio'
local asm = require 'hardware.pio.asm'
local list = require 'mlua.list'
local util = require 'mlua.util'
local package = require 'package'

package.loaded['hardware.pio.asm'] = nil  -- Reduce permanent memory usage

local function hex8(v) return ('0x%08x'):format(v) end

function test_run(t)
    local prog = asm.assemble(function(_ENV)
    loop:
        mov(isr, x)
        push()
        jmp(x_dec, loop)
    public(start)
    wrap_target()
        pull()
        mov(x, osr)
        jmp(x_dec, loop)
    end)

    -- Load the program.
    local inst = pio[0]
    local off = inst:add_program(prog)
    t:cleanup(function() inst:remove_program(prog, off) end)

    -- Configure and start the state machine.
    local sm = inst:sm(0)
    local cfg = prog:config(off)
    sm:init(prog.labels.start + off, cfg)
    t:cleanup(function() sm:set_enabled(false) end)
    sm:set_enabled(true)

    -- Exercise the program.
    local i = 7
    sm:put_blocking(i)
    while i > 0 do
        i = i - 1
        t:expect(t:expr(sm):get_blocking()):eq(i)
    end
end

function test_assemble(t)
    for name, fn in pairs(_ENV) do
        if not name:find('^pio_') then goto continue end
        t:context{program = name}
        local prog = asm.assemble(fn)
        local want = _ENV['want_' .. name]
        t:expect(prog):label('prog'):eq(want.instr, list.eq)
        t:expect(t:expr(prog).labels):eq(want.labels, util.table_eq)
        t:expect(t:expr(prog).origin):eq(want.origin)
        local cfg = prog:config(0)
        local wcfg = pio.get_default_sm_config()
        want.config(wcfg)
        t:expect(cfg:pinctrl()):label('pinctrl'):fmt(hex8):eq(wcfg:pinctrl())
        t:expect(cfg:execctrl()):label('execctrl'):fmt(hex8):eq(wcfg:execctrl())
        ::continue::
    end
end

function pio_addition(_ENV)
    -- https://github.com/raspberrypi/pico-examples/blob/sdk-1.5.1/pio/addition/addition.pio
    pull()
    mov(x, invert(osr))
    pull()
    mov(y, osr)
    jmp(test)
incr:
    jmp(x_dec, test)
test:
    jmp(y_dec, incr)
    mov(isr, invert(x))
    push()
end

want_pio_addition = {
    instr = {
        0x80a0, 0xa02f, 0x80a0, 0xa047, 0x0006, 0x0046, 0x0085, 0xa0c9,
        0x8020,
    },
    labels = {},
    config = function(cfg) cfg:set_wrap(0, 8) end,
}

function pio_apa102_mini(_ENV)
    -- https://github.com/raspberrypi/pico-examples/blob/sdk-1.5.1/pio/apa102/apa102.pio
    side_set(1)
    out(pins, 1)    side(0)
    nop()           side(1)
end

want_pio_apa102_mini = {
    instr = {0x6001, 0xb042},
    labels = {},
    config = function(cfg)
        cfg:set_wrap(0, 1)
        cfg:set_sideset(1, false, false)
    end,
}

function pio_clocked_input(_ENV)
    -- https://github.com/raspberrypi/pico-examples/blob/sdk-1.5.1/pio/clocked_input/clocked_input.pio
    wait(0, pin, 1)
    wait(1, pin, 1)
    in_(pins, 1)
end

want_pio_clocked_input = {
    instr = {0x2021, 0x20a1, 0x4001},
    labels = {},
    config = function(cfg) cfg:set_wrap(0, 2) end,
}

function pio_differential_manchester_tx(_ENV)
    -- https://github.com/raspberrypi/pico-examples/blob/sdk-1.5.1/pio/differential_manchester/differential_manchester.pio
    side_set(1)
public(start)
initial_high:
    out(x, 1)
    jmp(not_x, high_0)  side(1) delay(6)
high_1:
    nop()
    jmp(initial_high)   side(0) delay(6)
high_0:
    jmp(initial_low)            delay(7)

initial_low:
    out(x, 1)
    jmp(not_x, low_0)   side(0) delay(6)
low_1:
    nop()
    jmp(initial_low)    side(1) delay(6)
low_0:
    jmp(initial_high)           delay(7)
end

want_pio_differential_manchester_tx = {
    instr = {
        0x6021, 0x1e24, 0xa042, 0x1600, 0x0705, 0x6021, 0x1629, 0xa042,
        0x1e05, 0x0700,
    },
    labels = {start = 0},
    config = function(cfg)
        cfg:set_wrap(0, 9)
        cfg:set_sideset(2, true, false)
    end,
}

function pio_differential_manchester_rx(_ENV)
    -- https://github.com/raspberrypi/pico-examples/blob/sdk-1.5.1/pio/differential_manchester/differential_manchester.pio
public(start)
initial_high:
    wait(1, pin, 0)     delay(11)
    jmp(pin, high_0)
high_1:
    in_(x, 1)
    jmp(initial_high)
high_0:
    in_(y, 1)           delay(1)

wrap_target()
initial_low:
    wait(0, pin, 0)     delay(11)
    jmp(pin, low_1)
low_0:
    in_(y, 1)
    jmp(initial_high)
low_1:
    in_(x, 1)           delay(1)
wrap()
end

want_pio_differential_manchester_rx = {
    instr = {
        0x2ba0, 0x00c4, 0x4021, 0x0000, 0x4141, 0x2b20, 0x00c9, 0x4041,
        0x0000, 0x4121,
    },
    labels = {start = 0},
    config = function(cfg) cfg:set_wrap(5, 9) end,
}

function pio_i2c(_ENV)
    -- https://github.com/raspberrypi/pico-examples/blob/sdk-1.5.1/pio/i2c/i2c.pio
    side_set(1, pindirs)
do_nack:
    jmp(y_dec, entry_point)
    irq(wait, rel(0))

do_byte:
    set(x, 7)
bitloop:
    out(pindirs, 1)             delay(7)
    nop()               side(1) delay(2)
    wait(1, pin, 1)             delay(4)
    in_(pins, 1)                delay(7)
    jmp(x_dec, bitloop) side(0) delay(7)

    out(pindirs, 1)             delay(7)
    nop()               side(1) delay(7)
    wait(1, pin, 1)             delay(7)
    jmp(pin, do_nack)   side(0) delay(2)

entry_point:
wrap_target()
    out(x, 6)
    out(y, 1)
    jmp(not_x, do_byte)
    out(null, 32)
do_exec:
    out(exec, 16)
    jmp(x_dec, do_exec)
wrap()
end

want_pio_i2c = {
    instr = {
        0x008c, 0xc030, 0xe027, 0x6781, 0xba42, 0x24a1, 0x4701, 0x1743,
        0x6781, 0xbf42, 0x27a1, 0x12c0, 0x6026, 0x6041, 0x0022, 0x6060,
        0x60f0, 0x0050,
    },
    labels = {},
    config = function(cfg)
        cfg:set_wrap(12, 17)
        cfg:set_sideset(2, true, true)
    end,
}

function pio_i2c_set_scl_sda(_ENV)
    -- https://github.com/raspberrypi/pico-examples/blob/sdk-1.5.1/pio/i2c/i2c.pio
    side_set(1, opt)
    set(pindirs, 0) side(0) delay(7)
    set(pindirs, 1) side(0) delay(7)
    set(pindirs, 0) side(1) delay(7)
    set(pindirs, 1) side(1) delay(7)
end

want_pio_i2c_set_scl_sda = {
    instr = {0xf780, 0xf781, 0xff80, 0xff81},
    labels = {},
    config = function(cfg)
        cfg:set_wrap(0, 3)
        cfg:set_sideset(2, true, false)
    end,
}

function pio_quadrature_encoder(_ENV)
    -- https://github.com/raspberrypi/pico-examples/blob/sdk-1.5.1/pio/quadrature_encoder/quadrature_encoder.pio
    origin(0)

    jmp(update)
    jmp(decrement)
    jmp(increment)
    jmp(update)

    jmp(increment)
    jmp(update)
    jmp(update)
    jmp(decrement)

    jmp(decrement)
    jmp(update)
    jmp(update)
    jmp(increment)

    jmp(update)
    jmp(increment)
decrement:
    jmp(y_dec, update)

wrap_target()
update:
    mov(isr, y)
    push(noblock)

sample_pins:
    out(isr, 2)
    in_(pins, 2)
    mov(osr, isr)
    mov(pc, isr)

increment:
    mov(y, invert(y))
    jmp(y_dec, increment_cont)
increment_cont:
    mov(y, invert(y))
wrap()
end

want_pio_quadrature_encoder = {
    instr = {
        0x000f, 0x000e, 0x0015, 0x000f, 0x0015, 0x000f, 0x000f, 0x000e,
        0x000e, 0x000f, 0x000f, 0x0015, 0x000f, 0x0015, 0x008f, 0xa0c2,
        0x8000, 0x60c2, 0x4002, 0xa0e6, 0xa0a6, 0xa04a, 0x0097, 0xa04a,
    },
    labels = {},
    origin = 0,
    config = function(cfg) cfg:set_wrap(15, 23) end,
}
