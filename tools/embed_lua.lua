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
local chunk = loadfile(input_path)
local bin = string.dump(chunk)

-- Output the compiled chunk as C array data.
if output_path ~= "-" then
    io.output(output_path)
end
local offset = 0
while true do
    data = bin:sub(offset + 1, offset + 16)
    offset = offset + 16
    if data == "" then break end
    for i = 1, #data do
        io.write(string.format('0x%02x,', data:byte(i)))
    end
    io.write('\n')
end
io.write('0x00,\n')
io.close()
