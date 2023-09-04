-- Extract preprocessor symbols from the given file, and generate C symbol
-- definitions for integer values in a Lua table.

local io = require 'io'
local os = require 'os'
local string = require 'string'
local table = require 'table'

-- Same as assert(), but without adding error position information.
function check(...)
    local cond, msg = ...
    if cond then return ... end
    error(msg, 0)
end

function parse_defines(path)
    local syms = {}
    local f<close> = check(io.open(path, 'r'))
    for line in f:lines('l') do
        local sym = line:match('^#define ([a-zA-Z][a-zA-Z0-9_]*) .+$')
        if sym then syms[sym] = true end
    end
    return syms
end

function main(input_path, output_path)
    local input = parse_defines(input_path)
    local output<close> = check(io.open(output_path, 'w'))
    local syms = {}
    for sym in pairs(input) do syms[#syms + 1] = sym end
    table.sort(syms)
    for _, sym in ipairs(syms) do
        check(output:write(('#ifdef %s\n'):format(sym)))
        check(output:write(
            ('    MLUA_SYM_V(%s, integer, %s),\n'):format(sym, sym)))
        check(output:write('#endif\n'))
    end
end

local ok, err = pcall(main, ...)
if not ok then
    io.stderr:write(("ERROR: %s\n"):format(err))
    os.exit(1, true)
end
