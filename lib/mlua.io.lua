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

local codes = {
    NORM = '\x1b[0m',
    BOLD = '\x1b[1m',

    BLACK = '\x1b[30m',
    RED = '\x1b[31m',
    GREEN = '\x1b[32m',
    YELLOW = '\x1b[33m',
    BLUE = '\x1b[34m',
    MAGENTA = '\x1b[35m',
    CYAN = '\x1b[36m',
    WHITE = '\x1b[37m',

    ['+BLACK'] = '\x1b[90m',
    ['+RED'] = '\x1b[91m',
    ['+GREEN'] = '\x1b[92m',
    ['+YELLOW'] = '\x1b[93m',
    ['+BLUE'] = '\x1b[94m',
    ['+MAGENTA'] = '\x1b[95m',
    ['+CYAN'] = '\x1b[96m',
    ['+WHITE'] = '\x1b[97m',

    CLR = '\x1b[3J\x1b[H\x1b[2J'
}

-- Substitute @{...} ANSI codes in the given string.
function ansi(s) return s:gsub('@{([A-Z+]+)}', codes) end

-- ANSI-format a string.
function aformat(format, ...) return ansi(format):format(...) end

-- Print ANSI-formatted to stdout.
function aprintf(format, ...) return printf(ansi(format), ...) end

-- Print ANSI-formatted to an output stream.
function afprintf(out, format, ...) return fprintf(out, ansi(format), ...) end

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
