_ENV = mlua.Module(...)

local base = require 'hardware.base'
local gpio = require 'hardware.gpio'
local i2c = require 'hardware.i2c'
local addressmap = require 'hardware.regs.addressmap'
local regs = require 'hardware.regs.i2c'
local config = require 'mlua.config'
local thread = require 'mlua.thread'
local multicore = require 'pico.multicore'

local module_name = ...
local baudrate = 1000000
local slave_addr = 0x17

local master, slave = i2c[0], i2c[1]
local all_pins = {
    config.I2C_MASTER_SDA, config.I2C_MASTER_SCL,
    config.I2C_SLAVE_SDA, config.I2C_SLAVE_SCL,
}

function set_up(t)
    t:cleanup(function()
        for _, pin in ipairs(all_pins) do gpio.deinit(pin) end
        master:deinit()
        slave:deinit()
    end)
    for _, pin in ipairs(all_pins) do
        gpio.init(pin)
        gpio.pull_up(pin)
        t:assert(gpio.get_pad(pin), "Pin %s is forced low", pin)
    end

    for _, n in ipairs{'SDA', 'SCL'} do
        local mpin, spin = config['I2C_MASTER_' .. n], config['I2C_SLAVE_' .. n]
        local done<close> = function()
            gpio.set_oeover(mpin, gpio.OVERRIDE_NORMAL)
        end
        gpio.set_oeover(mpin, gpio.OVERRIDE_HIGH)
        t:assert(not gpio.get_pad(spin),
                 "Pin %s must be connected to pin %s", mpin, spin)
    end

    t:expect(t:expr(master):init(baudrate)):close_to_rel(baudrate, 0.01)
    gpio.set_function(config.I2C_MASTER_SDA, gpio.FUNC_I2C)
    gpio.set_function(config.I2C_MASTER_SCL, gpio.FUNC_I2C)

    t:expect(t:expr(slave):init(baudrate)):close_to_rel(baudrate, 0.01)
    slave:set_slave_mode(true, slave_addr)
    base.write32(slave:regs_base() + regs.IC_INTR_MASK_OFFSET,
                 regs.IC_INTR_MASK_M_RD_REQ_BITS)
    gpio.set_function(config.I2C_SLAVE_SDA, gpio.FUNC_I2C)
    gpio.set_function(config.I2C_SLAVE_SCL, gpio.FUNC_I2C)
end

function test_strict(t)
    if config.HASH_SYMBOL_TABLES ~= 0 then t:skip("Hashed symbol tables") end
    t:expect(function() return uart.UNKNOWN end)
        :label("module attribute access"):raises("undefined symbol")
    local inst = master
    t:expect(function() return inst.UNKNOWN end)
        :label("I2C instance attribute access"):raises("undefined symbol")
end

function test_index_base(t)
    for i = 0, i2c.NUM - 1 do
        local inst = i2c[i]
        t:expect(t:expr(inst):hw_index()):eq(i)
        t:expect(t:expr(inst):regs_base())
            :eq(addressmap[('I2C%s_BASE'):format(i)])
    end
end

local function run_slave(inst, on_receive, on_request)
    local i2c_base, in_progress = inst:regs_base(), false
    while true do
        while inst:get_read_available() > 0 do
            on_receive(inst, inst:read_data_cmd())
        end
        local intr_stat = base.read32(i2c_base + regs.IC_INTR_STAT_OFFSET)
        if intr_stat & regs.IC_INTR_STAT_R_RD_REQ_BITS ~= 0 then
            base.read32(i2c_base + regs.IC_CLR_RD_REQ_OFFSET)
            on_request(inst)
        end
        thread.yield()
    end
end

local function slave_handlers()
    local mem, addr = {}, nil
    return function(inst, data_cmd)
        if data_cmd > 0xff then  -- FIRST_DATA_BYTE is set
            addr = data_cmd & 0xff
        else
            mem[addr] = data_cmd
            addr = (addr + 1) & 0xff
        end
    end, function(inst)
        inst:write_byte_raw(mem[addr] or 0)
        addr = (addr + 1) & 0xff
    end
end

function test_master_slave_Y(t)
    -- Start the slave on core 1.
    multicore.launch_core1(module_name, 'core1_slave')
    t:cleanup(multicore.reset_core1)
    if yield_enabled() then master:enable_irq(true) end

    -- Write some data, then read it back.
    local data = 'abcdefghijklmnopqrstuvwxyz'
    t:expect(t:expr(master):write_blocking(slave_addr, '\x42' .. data, false))
        :eq(1 + #data)
    t:expect(t:expr(master):write_blocking(slave_addr, '\x42', true)):eq(1)
    t:expect(t:expr(master):read_blocking(slave_addr, 7, true)):eq('abcdefg')
    t:expect(t:expr(master):read_blocking(slave_addr, 19, false))
        :eq('hijklmnopqrstuvwxyz')
end

function core1_slave()
    multicore.set_shutdown_handler()
    run_slave(slave, slave_handlers())
end
