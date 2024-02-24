-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local pio = require 'hardware.pio'
local oo = require 'mlua.oo'
local string = require 'string'

-- TODO: Add origin()
-- TODO: Try to implement "foo:" syntax for labels. Or shorten label() to l()

local function raise(level, format, ...)
    return error(format:format(...), level)
end

local Program = oo.class('Program')

function assemble(prog) return Program(prog) end

local env_mt = {
    __index = function(p, k)
        local self = p.p
        local fn = self['i_' .. k]
        if not fn then return k end
        return function(...) return fn(self, ...) end
    end,
}

function Program:__init(prog)
    local env = setmetatable({p = self}, env_mt)
    self.labels = {}
    self.ss, self.sm, self.pc = 0, 0, 0
    prog(env)  -- Collect directives and labels
    if self.pc == 0 then return error("empty program", 0) end
    self.so = self.ss > 0 and (self.so or self.sm ~= (1 << self.pc) - 1) or nil
    self.pc, self.sm = 0
    prog(env)  -- Generate code
    self.pc = nil
    self.wrap_target = self.wrap_target or 0
    self.wrap = self.wrap or #self - 1
end

function Program:config(offset)
    local cfg = pio.get_default_sm_config()
    cfg:set_wrap(self.wrap_target + offset, self.wrap + offset)
    if self.ss > 0 then
        cfg:set_sideset(self.ss + (self.so and 1 or 0),
                        self.so, self.sd or false)
    end
    return cfg
end

-- Directives.

function Program:i_side_set(count, ...)
    if not self.sm then return end
    if self.pc ~= 0 then
        return error("side_set() after first instruction", 2)
    end
    if not (0 <= count and count <= 5) then
        return raise(2, "invalid side_set count: %s", count)
    end
    for _, f in ipairs{...} do
        if f == 'opt' then self.so = true
        elseif f == 'pindirs' then self.sd = true
        elseif f ~= nil then return raise(2, "invalid flag: %s", f) end
    end
    self.ss = count
end

function Program:i_wrap_target()
    if not self.sm then return end
    if self.wrap_target then return error("duplicate wrap_target()", 2) end
    self.wrap_target = self.pc
end

function Program:i_wrap()
    if not self.sm then return end
    if self.wrap then return error("duplicate wrap()", 2) end
    self.wrap = self.pc - 1
end

function Program:i_label(label)
    if not self.sm then return end
    if self.labels[label] then return raise(2, "duplicate label: %s", label) end
    self.labels[label] = self.pc
end

-- Instructions.

function Program:i_word(instr, label)
    if self.pc >= 32 then return error("program too long", 2) end
    self.pc = self.pc + 1
    if self.sm then return end
    local lpc = not label and 0 or self.labels[label]
    if not lpc then return raise(2, "unknown label: %s", label) end
    self[self.pc] = instr | lpc
end

function Program:i_nop(o, k) return self:i_word(0xa042) end

local jmp_conds = {not_x = 1, x_dec = 2, not_y = 3, y_dec = 4, x_not_y = 5,
                   pin = 6, not_osre = 7}

function Program:i_jmp(cond, label)
    local c = 0
    if not label then label = cond else c = jmp_conds[cond] end
    if not c then return raise(2, "invalid condition: %s", cond) end
    return self:i_word(0x0000 | (c << 5), label)
end

local wait_srcs = {gpio = 0, pin = 1, irq = 2}

function Program:i_wait(polarity, src, index)
    if type(src) == 'function' then src = 'irq' end
    local s = wait_srcs[src]
    if not s then return raise(2, "invalid source: %s", src) end
    return self:i_word(0x2000 | (polarity << 7) | (s << 5) | index)
end

local in_srcs = {pins = 0, x = 1, y = 2, null = 3, isr = 6, osr = 7}

function Program:i_in_(src, bcnt)
    local s = in_srcs[src]
    if not s then return raise(2, "invalid source: %s", src) end
    if not (0 < bcnt and bcnt <= 32) then
        return raise(2, "invalid bit count: %s", bcnt)
    end
    return self:i_word(0x4000 | (s << 5) | (bcnt & 0x1f))
end

local out_dests = {pins = 0, x = 1, y = 2, null = 3, pindirs = 4, pc = 5,
                   isr = 6, exec = 7}

function Program:i_out(dest, bcnt)
    local d = out_dests[dest]
    if not d then return raise(2, "invalid destination: %s", dest) end
    if not (0 < bcnt and bcnt <= 32) then
        return raise(2, "invalid bit count: %s", bcnt)
    end
    return self:i_word(0x6000 | (d << 5) | (bcnt & 0x1f))
end

function Program:i_push(...)
    return self:i_word(0x8000 | (self:_push_pull_flags(...) << 5))
end

function Program:i_pull(...)
    return self:i_word(0x8080 | (self:_push_pull_flags(...) << 5))
end

function Program:_push_pull_flags(...)
    local v = 1
    for _, f in ipairs{...} do
        if f == 'iffull' then v = v | 2
        elseif f == 'block' then v = v | 1
        elseif f == 'noblock' then v = v & ~1
        elseif f then return raise(4, "invalid flag: %s", f) end
    end
    return v
end

local mov_dests = {pins = 0, x = 1, y = 2, exec = 4, pc = 5, isr = 6, osr = 7}
local mov_ops = {['~'] = 1, [':'] = 2}
local mov_srcs = {pins = 0, x = 1, y = 2, null = 3, status = 5, isr = 6,
                  osr = 7}

function Program:i_mov(dest, src)
    local d = mov_dests[dest]
    if not d then return raise(2, "invalid destination: %s", dest) end
    local op, sn = src:match('^([~:]?)(.*)$')
    local o, s = mov_ops[op] or 0, mov_srcs[sn]
    if not s then return raise(2, "invalid source: %s", src) end
    return self:i_word(0xa000 | (d << 5) | (o << 3) | s)
end

function Program:i_invert(v) return '~' .. v end
function Program:i_reverse(v) return ':' .. v end

local irq_modes = {wait = 1, clear = 2}

function Program:i_irq(mode, index)
    if type(mode) == 'function' then mode = 'wait' end
    local m = 0
    if not index then index = mode else m = irq_modes[mode] end
    if not m then return raise(2, "invalid mode: %s", mode) end
    if (index & ~0x1f) ~= 0 then return raise(2, "invalid index: %s", index) end
    return self:i_word(0xc000 | (m << 5) | index)
end

function Program:i_rel(index)
    if (index & ~0x0f) ~= 0 then return raise(2, "invalid index: %s", index) end
    return 0x10 | index
end

local set_dests = {pins = 0, x = 1, y = 2, pindirs = 4}

function Program:i_set(dest, data)
    local d = set_dests[dest]
    if not d then return raise(2, "invalid destination: %s", dest) end
    if (data & ~0x1f) ~= 0 then return raise(2, "invalid data: %s", data) end
    return self:i_word(0xe000 | (d << 5) | data)
end

-- Modifiers.

function Program:i_side(value)
    if self.pc == 0 then return error("side() before first instruction", 2) end
    if self.ss == 0 then return error("no sideset", 2) end
    local m = (1 << self.ss) - 1
    if value > m then return raise(2, "invalid sideset: %s", value) end
    if self.sm then
        self.sm = self.sm | (1 << (self.pc - 1))
        return
    end
    local shift = 13 - (self.ss + (self.so and 1 or 0))
    self[self.pc] = (self[self.pc] & ~(m << shift)) | (self.so and 0x1000 or 0)
                    | (value << shift)
end

function Program:i_delay(value)
    if self.pc == 0 then return error("delay() before first instruction", 2) end
    if self.sm then return end
    local m = 0x1f >> (self.ss + (self.so and 1 or 0))
    if value & ~m ~= 0 then return raise(2, "invalid delay: %s", value) end
    self[self.pc] = (self[self.pc] & ~(m << 8)) | (value << 8)
end
