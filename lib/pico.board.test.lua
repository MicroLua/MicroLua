_ENV = mlua.Module(...)

local pico = require 'pico'
local board = require 'pico.board'

function test_DEFAULT_symbols(t)
    for k, v in pairs(pico) do
        if k:match('DEFAULT_.*') and v then
            t:expect(t:expr(board)['PICO_' .. k]):eq(v)
        end
    end
end
