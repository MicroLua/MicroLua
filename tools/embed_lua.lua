-- Convert a .lua file to C array data, so that it can be #included within an
-- array initializer.
--
-- Lua seems to rely on a terminating zero byte when loading chunks in text
-- mode, even if it isn't included in the chunk size. This is likely a bug. We
-- work around it by appending a zero byte to the output.

local io = require 'io'
local string = require 'string'

local input_path, output_path = ...

-- Compile the input file.
if input_path == '-' then input_path = nil end
local chunk = assert(loadfile(input_path))
local bin = string.dump(chunk)

-- Output the compiled chunk as C array data.
if output_path ~= "-" then
    io.output(output_path)
end
local offset = 0
while true do
    if offset >= #bin then break end
    for i = 1, 16 do
        local v = bin:byte(offset + i)
        if v == nil then break end
        assert(io.write(string.format('0x%02x,', v)))
    end
    assert(io.write('\n'))
    offset = offset + 16
end
assert(io.write('0x00,\n'))
assert(io.close())
