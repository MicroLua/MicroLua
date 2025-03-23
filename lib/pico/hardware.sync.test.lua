-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local irq = require 'hardware.irq'
local sync = require 'hardware.sync'
local thread = require 'mlua.thread'

function test_disable_restore_interrupts(t)
    local num = irq.user_irq_claim_unused()
    t:cleanup(function() irq.user_irq_unclaim(num) end)
    local triggered = false
    irq.set_handler(num, function(n) triggered = true end)
    t:cleanup(function() irq.remove_handler(num) end)
    irq.set_enabled(num, true)
    t:cleanup(function() irq.set_enabled(num, false) end)

    local save = sync.save_and_disable_interrupts()
    irq.set_pending(num)
    thread.yield()
    t:expect(not triggered, "IRQ has triggered")
    sync.restore_interrupts(save)
    thread.yield()
    t:expect(triggered, "IRQ hasn't triggered")
end
