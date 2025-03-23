-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local unique_id = require 'pico.unique_id'
local string = require 'string'

function test_board_id(t)
    local id = unique_id.get_unique_board_id()
    t:expect(#id == unique_id.BOARD_ID_SIZE,
             "Unexpected size: got %d, want %d", #id, unique_id.BOARD_ID_SIZE)

    local sid = unique_id.get_unique_board_id_string()
    local want = id:gsub('(.)', function(c)
        return ('%02X'):format(c:byte(1))
    end)
    t:expect(sid == want, "Unexpected string ID: got %q, want %q", sid, want)
end
