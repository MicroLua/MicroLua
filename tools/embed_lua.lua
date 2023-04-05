-- Convert a .lua file to C array data, so that it can be #included within an
-- array initializer.
--
-- Lua seems to rely on a terminating zero byte when loading chunks in text
-- mode, even if it isn't included in the chunk size. This is likely a bug. We
-- work around it by appending a zero byte to the output.

local io = require 'io'
local package = require 'package'
local string = require 'string'

local input_path, output_path = ...

local dirsep = package.config:sub(1, 1)

function basename(path)
    local i = path:find(string.format('%s[^%s]*$', dirsep, dirsep))
    if i ~= nil then return path:sub(i + 1) end
    return path
end

-- Compile the input file.
local name = '=<unknown>'
if input_path ~= '-' then
    io.input(input_path)
    name = '@' .. basename(input_path)
end
local chunk = assert(load(function() return io.read(4096) end, name))
local bin = string.dump(chunk)

-- Output the compiled chunk as C array data.
if output_path ~= '-' then io.output(output_path) end
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
