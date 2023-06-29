-- Input / output utilities.

_ENV = mlua.Module(...)

local oo = require 'mlua.oo'
local string = require 'string'
local table = require 'table'

-- Read from stdin.
function read(...) return stdin:read(...) end

-- Write to stdout.
function write(...) return stdout:write(...) end

-- Print formatted to stdout.
function printf(format, ...) return stdout:write(format:format(...)) end

-- Print formatted to an output stream.
function fprintf(out, format, ...) return out:write(format:format(...)) end

-- A writer that collects writes and can replay them.
Buffer = oo.class('Buffer')

-- Return true iff the buffer is empty.
function Buffer:is_empty() return (self[0] or 0) == 0 end

-- Write data to the writer.
function Buffer:write(...)
    local len = self[0] or 0
    for i = 1, select('#', ...) do
        local data = select(i, ...)
        if #data > 0 then
            len = len + 1
            self[len] = data
        end
    end
    if len > 0 then self[0] = len end
end

-- Replay the written data to the given writer.
function Buffer:replay(w)
    for _, data in ipairs(self) do w:write(data) end
end

-- Return the content of the buffer as a string.
function Buffer:__tostring() return table.concat(self) end
