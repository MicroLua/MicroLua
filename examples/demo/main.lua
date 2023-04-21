_ENV = require 'module'(...)

local math = require 'math'
local pico = require 'pico'
local platform = require 'pico.platform'
local time = require 'pico.time'
local string = require 'string'

function main()
    print("============================\nIn main()")
    print(string.format("chip version: %d", platform.rp2040_chip_version()))
    print(string.format("rom version: %d", platform.rp2040_rom_version()))
    print(string.format("core: %d", platform.get_core_num()))
    print(string.format("SDK: %s", pico.SDK_VERSION_STRING))

    for i = 1, 5 do
        local t = time.get_absolute_time()
        print(string.format("time: %s us (%d ms)", t, time.to_ms_since_boot(t)))
        time.sleep_ms(200)
    end

    print(1, 2, 3, "soleil")
    while true do
        local line = stdin:read()
        stdout:write(string.format("You entered: %s", line))
    end
end
