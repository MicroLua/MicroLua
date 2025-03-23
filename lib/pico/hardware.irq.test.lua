-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local irq = require 'hardware.irq'
local thread = require 'mlua.thread'
local list = require 'mlua.list'
local string = require 'string'

function test_pending(t)
    local num = irq.user_irq_claim_unused()
    t:cleanup(function() irq.user_irq_unclaim(num) end)

    irq.clear(num)
    t:expect(t.expr(irq).is_pending(num)):eq(false)
    irq.set_pending(num)
    t:expect(t.expr(irq).is_pending(num)):eq(true)
    irq.clear(num)
    t:expect(t.expr(irq).is_pending(num)):eq(false)
end

function test_user_irqs(t)
    local rounds = 10

    -- Set up IRQ handlers.
    local irqs, nums, log = list(), '', nil
    while true do
        local num = irq.user_irq_claim_unused(false)
        if num < 0 then break end
        t:cleanup(function() irq.user_irq_unclaim(num) end)
        irq.set_handler(num, function(n) log = log .. ('%s '):format(n) end)
        t:cleanup(function() irq.remove_handler(num) end)
        irq.set_enabled(num, true)
        t:cleanup(function() irq.set_enabled(num, false) end)
        irqs:append(num)
        nums = nums .. ('%s '):format(num)
    end
    t:assert(#irqs > 0, "No user IRQs available")

    -- Trigger IRQs.
    for i = 1, rounds do
        log = ''
        for _, num in ipairs(irqs) do irq.set_pending(num) end
        thread.yield()
        t:expect(log == nums, "Unexpected sequence on round %s: %s", i, log)
    end
end
