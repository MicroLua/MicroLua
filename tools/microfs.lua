-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local io = require 'io'
local math = require 'math'
local block_mem = require 'mlua.block.mem'
local cli = require 'mlua.cli'
local errors = require 'mlua.errors'
local fs = require 'mlua.fs'
local lfs = require 'mlua.fs.lfs'
local mio = require 'mlua.io'
local list = require 'mlua.list'
local mem = require 'mlua.mem'
local shell = require 'mlua.shell'
local uf2 = require 'mlua.uf2'
local util = require 'mlua.util'
local os = require 'os'
local string = require 'string'
local table = require 'table'

local printf = mio.printf
local check = util.check
local raise = util.raise

-- Return a view into a slice of a list.
local function slice(list, from)
    local o = from - 1
    return setmetatable({}, {
        __index = function(_, k) return list[k + o] end,
        __len = function(_, k)
            local len = #list - o
            return len < 0 and 0 or len
        end,
    })
end

-- Run a command as a sub-process and capture its output.
local function run(cmd)
    local parts = list()
    for _, p in ipairs(cmd) do parts:append(shell.quote(p)) end
    local f<close> = check(io.popen(parts:concat(' ')))
    local output = f:read('a')
    local ok, res, num = f:close()
    if not ok then
        stderr:write(output)
        raise("%s failed with %s %s", cmd[1],
              res == 'exit' and "exit code" or "signal", num)
    end
    return output
end

-- Read the content of an UF2 file into a memory buffer.
local function read_uf2(path)
    local f<close> = check(io.open(path, 'rb'))
    local blocks, start, size = list()
    while true do
        local block = f:read(uf2.block_size)
        if not block then break end
        local b = check(uf2.parse(block))
        if b.flags & uf2.flag_noflash == 0 then
            blocks:append(b)
            local addr, psize = b.target_addr, b.payload_size
            local e = addr + psize
            if not start then
                start, size = addr, psize
            elseif math.ult(addr, start) then
                start, size = addr, size + (start - addr)
            elseif math.ult(start + size, e) then
                size = e - start
            end
        end
    end
    local buf = mem.alloc(size)
    for _, b in ipairs(blocks) do
        buf:write(b.data:sub(1, b.payload_size), b.target_addr - start)
    end
    return buf, start
end

-- embedded drive:  0x101e7000-0x10200000 (100K): lfs:label
local drive_pat = '\n%s*embedded drive:%s*0x([0-9a-fA-F]+)%-0x([0-9a-fA-F]+)'
                  .. '%s[^:]*:%s*(.-)\n'

local function add_picotool_opts(cmd, opts)
    if opts.bus then cmd:append('--bus', opts.bus) end
    if opts.addr then cmd:append('--address', opts.addr) end
end

-- Get information about an embedded drive.
local function find_drive(opts)
    local cmd = list{opts.picotool, 'info', '-a'}
    add_picotool_opts(cmd, opts)
    local output = run(cmd)
    local saddr, eaddr, label = output:match(drive_pat)
    local addr = tonumber(saddr, 16)
    return addr, tonumber(eaddr, 16) - addr, label
end

-- Read a range from flash memory.
local function read_range(opts, addr, size)
    local tmp = os.tmpname()
    local done<close> = function() os.remove(tmp) end
    local cmd = list{opts.picotool, 'save', '-r', ('0x%08x'):format(addr),
                     ('0x%08x'):format(addr + size)}
    add_picotool_opts(cmd, opts)
    cmd:append(tmp, '-t', 'uf2')
    run(cmd)
    return read_uf2(tmp)
end

-- Walk a filesystem tree, calling fn for each entry.
local function walk(efs, path, fn)
    local d<close>, msg, err = efs:opendir(path)
    if not d then
        if err ~= errors.ENOTDIR then raise(msg) end
        local name, type, size = check(efs:stat(path))
        fn(fs.join(path, name), type, size)
        return 1
    end
    local cnt = 0
    while true do
        local name, type, size = check(d:read())
        if name == true then break end
        cnt = cnt + 1
        if name ~= '.' and name ~= '..' then
            local p = fs.join(path, name)
            if type == fs.TYPE_DIR then size = walk(efs, p, fn) end
            fn(p, type, size)
        end
    end
    return cnt
end

function cmd_list(opts, args)
    cli.parse_opts(opts, {
        address = cli.str_opt(nil),
        block_size = cli.num_opt(4096),
        bus = cli.str_opt(nil),
        picotool = cli.str_opt('picotool'),
    })

    -- Find the block device on the device.
    local addr, size, fs_type = find_drive(opts)
    if not addr then raise("no embedded drive found") end
    -- printf("Block device at 0x%08x, size: 0x%08x, %s\n", addr, size, fs_type)

    -- Read the block device content.
    local data, start = read_range(opts, addr, size)
    if start ~= addr then
        raise("UF2 start (0x%08x) != binary info start (0x%08x)", start, addr)
    end
    if #data ~= size then
        raise("UF2 size (%s) != binary info size (%s)", #data, size)
    end

    -- Mount the filesystem
    local efs<close> = lfs.new(block_mem.new(data, 256, opts.block_size))
    local ok, msg = efs:mount()
    if not ok then raise("failed to mount filesystem: %s", msg) end

    -- List filesystem entries.
    if #args == 0 then args = list{'/'} end
    for _, arg in ipairs(args) do
        local entries = list()
        walk(efs, arg, function(path, type, size)
            entries:append({path = path,
                            type = type == fs.TYPE_DIR and 'd' or nil,
                            size = size})
        end)
        table.sort(entries, util.table_comp{'path'})
        local sd = 1
        for _, e in ipairs(entries) do
            local l = #tostring(e.size)
            if l > sd then sd = l end
        end
        local fmt = ("%%s %%%dd %%s\n"):format(sd)
        for _, e in ipairs(entries) do
            printf(fmt, e.type or '-', e.size or 0, e.path)
        end
    end
end

function main()
    local opts, args = cli.parse_args(arg)
    local cmd = args[1]
    local fn = _ENV['cmd_' .. cmd]
    if not fn then raise("unknown command: %s", cmd) end
    return fn(opts, slice(args, 2))
end
