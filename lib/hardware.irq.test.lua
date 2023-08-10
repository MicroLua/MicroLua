_ENV = mlua.Module(...)

local irq = require 'hardware.irq'
local thread = require 'mlua.thread'
local list = require 'mlua.list'
local string = require 'string'

function test_pending(t)
    local num = irq.user_irq_claim_unused()
    t:cleanup(function() irq.user_irq_unclaim(num) end)

    irq.clear(num)
    t:expect(t:expr(irq).is_pending(num)):eq(false)
    irq.set_pending(num)
    t:expect(t:expr(irq).is_pending(num)):eq(true)
    irq.clear(num)
    t:expect(t:expr(irq).is_pending(num)):eq(false)
end

function test_user_irqs(t)
    local rounds = 10

    -- Claim user IRQs.
    local irqs, nums = list(), ''
    for i = 1, irq.NUM_USER_IRQS do
        local num = irq.user_irq_claim_unused()
        irq.clear(num)
        irq.enable_user_irq(num)
        t:cleanup(function()
            irq.enable_user_irq(num, false)
            irq.user_irq_unclaim(num)
        end)
        irqs:append(num)
        nums = nums .. ('%s '):format(num)
    end

    -- Set up IRQ handlers.
    local log
    local handlers<close> = thread.Group()
    for _, num in ipairs(irqs) do
        handlers:start(function()
            for i = 1, rounds do
                irq.wait_user_irq(num)
                log = log .. ('%s '):format(num)
            end
        end)
    end

    -- Trigger IRQs.
    for i = 1, rounds do
        log = ''
        for _, num in ipairs(irqs) do irq.set_pending(num) end
        thread.yield()
        t:expect(log == nums, "Unexpected sequence on round %s: %s", i, log)
    end
end
