-- An enhanced io module.

_ENV = require 'module'(...)

local io = require 'io'
local oo = require 'oo'
local string = require 'string'

stdin = io.stdin
stdout = io.stdout
stderr = io.stderr

-- Read from stdin.
function read(...) return stdin:read(...) end

-- Write to stdout.
function write(...) return stdout:write(...) end

-- Replace the global print function so that it uses stdout as defined in this
-- module, instead of the C stdout.
function _G.print(...)
    for i = 1, select('#', ...) do
        local s = tostring(select(i, ...))
        if i > 1 then stdout:write('\t') end
        stdout:write(s)
    end
    stdout:write('\n')
end

-- Print formatted to stdout.
function printf(format, ...)
    stdout:write(string.format(format, ...))
end

-- Print formatted to an output stream.
function fprintf(out, format, ...)
    out:write(string.format(format, ...))
end

-- A writer that collects all writes and can replay them.
Buffer = oo.class('Buffer')

-- Return true iff the buffer is empty.
function Buffer:is_empty() return #self == 0 end

-- Write data to the writer.
function Buffer:write(...)
    for i = 1, select('#', ...) do
        self[#self + 1] = select(i, ...)
    end
end

-- Replay the written data to the given writer.
function Buffer:replay(w)
    for _, data in ipairs(self) do
        w:write(data)
    end
end
