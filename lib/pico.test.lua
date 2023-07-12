_ENV = mlua.Module(...)

local pico = require 'pico'

function set_up(t)
    t:printf("Lua: %s, SDK: %s, Flash: %08x, binary: %08x - %08x (%.1f%%)\n",
             _RELEASE:gsub('[^ ]+ (.*)', '%1'), pico.SDK_VERSION_STRING,
             pico.FLASH_SIZE_BYTES,
             pico.flash_binary_start, pico.flash_binary_end,
             (pico.flash_binary_end - pico.flash_binary_start)
             / pico.FLASH_SIZE_BYTES * 100)
end

function test_flash(t)
    t:expect(pico.flash_binary_start <= pico.flash_binary_end,
             "flash_binary_start > flash_binary_end")
end
