-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

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

ansi_tags = {
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

    CLR = '\x1b[3J\x1b[H\x1b[2J',
    CLREOS = '\x1b[J',
    SAVE = '\x1b[s',
    RESTORE = '\x1b[u',
    SHOW = '\x1b[?25h',
    HIDE = '\x1b[?25l',
}

function empty_tags() return '' end

-- Substitute @{...} tags in the given string.
function ansi(s, tags) return s:gsub('@{([A-Z_+]+)}', tags or ansi_tags) end

-- ANSI-format a string.
function aformat(format, ...) return ansi(format):format(...) end

-- Print ANSI-formatted to stdout.
function aprintf(format, ...) return printf(ansi(format), ...) end

-- Print ANSI-formatted to an output stream.
function afprintf(out, format, ...) return fprintf(out, ansi(format), ...) end

-- Read exactly "len" bytes from a reader. May return fewer bytes if the reader
-- reaches EOF.
function read_all(reader, len, ...)
    local parts, cnt = {}, 0
    while not len or cnt < len do
        local data, err = reader:read(len and len - cnt or 200, ...)
        if not data then return data, err end
        local dl = #data
        if dl == 0 then break end
        table.insert(parts, data)
        cnt = cnt + dl
    end
    return table.concat(parts)
end

-- Read one newline-terminated line from a reader. May return an unterminated
-- line if the reader reaches EOF. This function is inefficient, because it
-- reads one character at a time.
function read_line(reader, ...)
    local parts = {}
    while true do
        local c, err = reader:read(1, ...)
        if not c then return c, err end
        if c == '' then break end
        table.insert(parts, c)
        if c == '\n' then break end
    end
    return table.concat(parts)
end

-- A writer that collects writes and can replay them.
Recorder = oo.class('Recorder')

-- Return true iff the buffer is empty.
function Recorder:is_empty() return (self.n or 0) == 0 end

-- Write data to the writer.
function Recorder:write(...)
    local len = self.n or 0
    for i = 1, select('#', ...) do
        local data = select(i, ...)
        if #data > 0 then
            len = len + 1
            self[len] = data
        end
    end
    if len > 0 then self.n = len end
end

-- Replay the written data to the given writer.
function Recorder:replay(w)
    for _, data in ipairs(self) do w:write(data) end
end

-- Return the content of the buffer as a string.
function Recorder:__tostring() return table.concat(self) end

-- A writer that indents written lines, except empty ones.
Indenter = oo.class('Indenter')

function Indenter:__init(w, indent)
    self._w, self._indent, self._nl = w, indent, '\n' .. indent .. '%1'
end

function Indenter:write(...)
    for i = 1, select('#', ...) do
        local data = select(i, ...)
        if #data > 0 then
            data = data:gsub('\n([^\n]+)', self._nl)
            if not self._nsol and data:byte(1) ~= 10 then
                data = self._indent .. data
            end
            self._w:write(data)
            self._nsol = data:byte(#data) ~= 10
        end
    end
end
