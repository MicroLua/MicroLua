-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local standard_link = require 'pico.standard_link'

function set_up(t)
    local sections = {
        {'FLASH',
            'flash_binary_start',
            'logical_binary_start',
            'binary_info_header_end',
            'binary_info_start',
            'binary_info_end',
            'data_source',
            'scratch_x_source',
            'scratch_y_source',
            'flash_binary_end',
        },
        {'RAM',
            'ram_vector_table',
            'data_start',
            'data_end',
            'bss_start',
            'bss_end',
            'heap_start',
            'heap_limit',
            'heap_end',

        },
        {'SCRATCH_X',
            'scratch_x_start',
            'scratch_x_end',
        },
        {'SCRATCH_Y',
            'scratch_y_start',
            'scratch_y_end',
        },
        {'Stack',
            'stack1_bottom',
            'stack1_top',
            'stack_bottom',
            'stack_top',
        },
    }
    for _, section in ipairs(sections) do
        local name = section[1]
        if name == name:upper() then
            t:printf("@{+CYAN}%s:@{NORM} %08x, size: %x\n", name,
                     standard_link[name] - pointer(0),
                     standard_link[name .. '_SIZE'])
        else t:printf("@{+CYAN}%s:@{NORM}\n", name) end
        for i = 2, #section do
            local sym = section[i]
            t:printf("  %-23s: %08x\n", sym, standard_link[sym] - pointer(0))
        end
    end
end
