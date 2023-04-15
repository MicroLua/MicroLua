_ENV = require 'module'(...)

local int64 = require 'int64'
local math = require 'math'
local pico = require 'pico'
local platform = require 'pico.platform'
local time = require 'pico.time'
local string = require 'string'

function main()
    print("============================\nIn main()")
    print(string.format("max integer: %d", math.maxinteger))
    print(string.format("max float: %g", 3.402823466e38))
    print(string.format("max double: %g", 1.7976931348623158e308))
    print()
    print(string.format("min int64: %s", int64.min))
    print(string.format("           %s", int64.min:hex()))
    print(string.format("max int64: %s", int64.max))
    print(string.format("           %s", int64.max:hex()))
    print(string.format("int64(true): %s", int64(true)))
    print(string.format("int64(123): %s", int64(123)))
    print(string.format("int64(123.0): %s", int64(123.0)))
    print(string.format("int64('123'): %s", int64('123')))
    print(string.format("int64('-9223372036854775808'): %s",
                        int64('-9223372036854775808')))
    print(string.format("int64('9223372036854775807'): %s",
                        int64('9223372036854775807')))
    print()
    print(int64(123) + int64(456))
    print(123 + int64(456))
    print(123.0 + int64(456))
    print(int64(123) + 456)
    print(int64(123) + 456.0)
    print(-int64(123))
    print(~int64(0))
    print(int64(123) << 1)
    print(int64(123) >> 1)
    print(tostring(int64(123)))
    print(getmetatable(int64(123)))
    print(int64(123):hex())
    -- print(int64(123) .. int64(456))
    -- print(123 .. int64(456))
    -- print(123.0 .. int64(456))
    print(int64(123):hex())
    print(int64(123):hex(8))
    print(int64(123):tointeger())
    print(int64.max:tointeger())
    print(int64(0x12345678, 0x24681357):hex())
    print(int64(0xabcdef12, 0xfedcba21):hex())
    print(int64('101100', 2):hex())
    print(int64('deadbeef', 16):hex())
    print(int64('xyz', 36):hex())

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
