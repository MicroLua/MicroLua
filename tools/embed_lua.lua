-- Convert a .lua file to C array data, so that it can be #included within an
-- array initializer.
--
-- Lua seems to rely on a terminating zero byte when loading chunks in text
-- mode, even if it isn't included in the chunk size. This is likely a bug. We
-- work around it by appending a zero byte to the output.

local io = require 'io'
local string = require 'string'

input_path, output_path = ...

if input_path ~= "-" then
    io.input(input_path)
end
if output_path ~= "-" then
    io.output(output_path)
end
while true do
    data = io.read(16)
    if data == fail then break end
    for i = 1, #data do
        io.write(string.format('0x%02x,', data:byte(i)))
    end
    io.write('\n')
end
io.write('0x00,\n')
io.close()
