-- An enhanced io module.

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

-- A writer that collects all writes and can replay them.
Buffer = oo.class('Buffer')

function Buffer:__init() self[0] = 0 end

-- Return true iff the buffer is empty.
function Buffer:is_empty() return self[0] == 0 end

-- Write data to the writer.
function Buffer:write(...)
    local len = self[0]
    for i = 1, select('#', ...) do
        len = len + 1
        self[len] = select(i, ...)
    end
    self[0] = len
end

-- Replay the written data to the given writer.
function Buffer:replay(w)
    for _, data in ipairs(self) do w:write(data) end
end

-- Return the content of the buffer as a string.
function Buffer:__tostring() return table.concat(self) end
