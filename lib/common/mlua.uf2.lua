-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local string = require 'string'

local magic_start0 = 0x0a324655
local magic_start1 = 0x9e5d5157
local magic_end = 0x0ab16f30
flag_noflash = 0x00000001
flag_file_container = 0x00001000
flag_family_id_present = 0x00002000
flag_md5_present = 0x00004000
flag_ext_present = 0x00008000

family_id_rp2040 = 0xe48bff56

local struct = '<I4I4I4I4I4I4I4I4c476I4'
block_size = struct:packsize()
assert(block_size == 512, "unexpected UF2 block size")

-- Parse an UF2 block.
function parse(block, start)
    if not start then start = 1 end
    if start + block_size - 1 > #block then return nil, "incomplete block" end
    local ms0, ms1, f, a, s, bno, nb, res, d, me = struct:unpack(block, start)
    if ms0 ~= magic_start0 then
        return nil, "invalid UF2 block (magic_start0)"
    end
    if ms1 ~= magic_start1 then
        return nil, "invalid UF2 block (magic_start1)"
    end
    if me ~= magic_end then return nil, "invalid UF2 block (magic_end)" end
    return {flags = f, target_addr = a, payload_size = s,
            block_no = bno, num_blocks = nb, reserved = res, data = d}
end

-- Serialize an UF2 block.
function serialize(block)
    for _, field in ipairs{'target_addr', 'block_no', 'num_blocks', 'data'} do
        if not block[field] then return nil, ('missing %s'):format(field) end
    end
    return struct:pack(magic_start0, magic_start1, block.flags or 0,
                       block.target_addr, #block.data, block.block_no,
                       block.num_blocks, block.reserved or 0, block.data,
                       magic_end)
end
