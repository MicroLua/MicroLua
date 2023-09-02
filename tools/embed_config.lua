-- Generate registration entries for a list of C symbols.

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

function main(output_path, ...)
    local output<close> = check(io.open(output_path, 'w'))
    for _, sym in ipairs(table.pack(...)) do
        local name, typ, value = sym:match('^([^:]+):([^=]+)=(.*)$')
        if not name then
            error(("invalid symbol definition: %s"):format(sym), 0)
        end
        check(output:write(
            ('    MLUA_SYM_V(%s, %s, %s),\n'):format(name, typ, value)))
    end
end

local ok, err = pcall(main, ...)
if not ok then
    io.stderr:write(("ERROR: %s\n"):format(err))
    os.exit(1, true)
end
