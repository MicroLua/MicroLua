-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local io = require 'io'
local math = require 'math'
local block_mem = require 'mlua.block.mem'
local cli = require 'mlua.cli'
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

local function device_opt(def)
    return function(n, v)
        if v == nil then return def end
        if v == true then return {} end
        local bus = v:match('^([0-9]+)$')
        if bus then
            bus = tonumber(bus, 10)
            if bus then return {bus = bus} end
        end
        local bus, addr = v:match('^([0-9]+):([0-9]+)$')
        if bus and addr then
            bus, addr = tonumber(bus, 10), tonumber(addr, 10)
            if bus and addr then return {bus = bus, addr = addr} end
        end
        raise("--%s: invalid argument: %s", n, v)
    end
end

-- Read the content of a file.
local function read_file(path)
    local f<close> = check(io.open(path, 'rb'))
    return check(f:read('a'))
end

-- Write the content of a file.
local function write_file(path, data)
    local f<close> = check(io.open(path, 'wb'))
    return check(f:write(data))
end

-- Read an UF2 file into a memory buffer.
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
        mem.write(buf, b.data:sub(1, b.payload_size), b.target_addr - start)
    end
    return buf, start
end

-- Write a memory buffer to an UF2 file.
local function write_uf2(data, addr, path)
    local f<close> = check(io.open(path, 'wb'))
    local block = {flags = uf2.flag_family_id_present,
                   reserved = uf2.family_id_rp2040,
                   num_blocks = (#data + 255) // 256}
    for i = 0, block.num_blocks - 1 do
        local off = i * 256
        block.target_addr = addr + off
        block.block_no = i
        block.data = mem.read(data, off, math.min(256, #data - off))
        check(f:write(check(uf2.serialize(block))))
    end
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

-- embedded drive:  0x101e7000-0x10200000 (100K): lfs:label
local drive_pat = '\n%s*embedded drive:%s*0x([0-9a-fA-F]+)%-0x([0-9a-fA-F]+)'
                  .. '%s[^:]*:%s*(.-)\n'

-- Add options relevant to picotool to a picotool command.
local function add_picotool_opts(cmd, opts)
    local dev, prog = opts.device, opts.program
    if dev then
        if dev.bus then cmd:append('--bus', dev.bus) end
        if dev.address then cmd:append('--address', dev.addr) end
        cmd:append('--force-no-reboot')
    elseif prog then
        cmd:append(prog)
    end
end

-- Get information about an embedded drive.
local function find_flash_drive(opts)
    local cmd = list{opts.picotool, 'info', '--all'}
    add_picotool_opts(cmd, opts)
    local output = run(cmd)
    local saddr, eaddr, label = output:match(drive_pat)
    local addr = tonumber(saddr, 16)
    return addr, tonumber(eaddr, 16) - addr, label
end

-- Read a range from flash memory.
local function read_flash_range(opts, addr, size)
    local tmp = os.tmpname()
    local done<close> = function() os.remove(tmp) end
    local cmd = list{opts.picotool, 'save', '--range', ('0x%08x'):format(addr),
                     ('0x%08x'):format(addr + size)}
    add_picotool_opts(cmd, opts)
    cmd:append(tmp, '-t', 'uf2')
    run(cmd)
    local data, start = read_uf2(tmp)
    if start ~= addr then
        raise("UF2 start (0x%08x) != requested start (0x%08x)", start, addr)
    end
    if #data ~= size then
        raise("UF2 size (%s) != requested size (%s)", #data, size)
    end
    return data
end

-- Write a range to flash memory.
local function write_flash_range(opts, path)
    local cmd = list{opts.picotool, 'load', '--update'}
    cmd:append(path, '-t', 'uf2')
    add_picotool_opts(cmd, opts)
    run(cmd)
end

-- Walk a filesystem tree, calling fn for each entry.
local function walk(efs, path, fn)
    local name, type, size = check(efs:stat(path))
    if type ~= fs.TYPE_DIR then
        fn(path, type, size)
        return 1
    end
    local function recurse(path)
        local cnt = 0
        for name, type, size in check(efs:list(path)) do
            cnt = cnt + 1
            if name ~= '.' and name ~= '..' then
                local p = fs.join(path, name)
                if type == fs.TYPE_DIR then size = recurse(p) end
                fn(p, type, size)
            end
        end
        return cnt
    end
    return recurse(path)
end

-- Create parent directories of the given path, if necessary.
local function create_parent_dirs(efs, path)
    local parents = list()
    while true do
        path = fs.split(path)
        if path == ('/'):rep(#path) then break end
        if efs:stat(path) then break end
        parents:append(path)
    end
    if #parents == 0 then return false end
    for i = #parents, 1, -1 do
        local dir = parents[i]
        printf("Creating directory %s\n", dir)
        check(efs:mkdir(dir))
    end
    return true
end

local function fs_op_list(efs, path)
    printf("Listing %s\n", path)
    local entries = list()
    walk(efs, path, function(path, type, size)
        entries:append({path = path, type = type == fs.TYPE_DIR and 'd' or nil,
                        size = size})
    end)
    entries:sort(util.table_comp{'path'})
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

local function fs_op_read(efs, src, dst)
    -- TODO: Handle the case where src is a directory => recurse
    printf("Reading %s to %s\n", src, dst)
    local f<close> = check(efs:open(dst, fs.O_RDONLY))
    local size = check(f:size())
    write_file(dst, check(f:read(data)))
end

local function fs_op_write(efs, src, dst)
    -- TODO: Handle the case where src is a directory => recurse
    local data = read_file(src)
    create_parent_dirs(efs, dst)
    printf("Writing %s to %s\n", src, dst)
    local f<close> = check(efs:open(dst, fs.O_WRONLY | fs.O_CREAT | fs.O_TRUNC))
    check(f:write(data))
end

local function fs_op_mkdir(efs, path)
    create_parent_dirs(efs, path)
    printf("Creating directory %s\n", path)
    local ok, typ = efs:stat(path)
    if ok and typ == fs.TYPE_DIR then return 1 end
    check(efs:mkdir(path))
end

local function fs_op_remove(efs, path)
    printf("Removing %s\n", path)
    local ok, typ = efs:stat(path)
    if not ok then return 1 end
    if typ == fs.TYPE_DIR then
        walk(efs, path, function(path, type, size) check(efs:remove(path)) end)
    end
    if path == ('/'):rep(#path) then return 1 end
    check(efs:remove(path))
end

local function fs_op_rename(efs, old, new)
    create_parent_dirs(efs, new)
    printf("Moving %s to %s\n", old, new)
    check(efs:rename(old, new))
end

local fs_ops = {
    list = {fs_op_list, 1},
    read = {fs_op_read, 2},
    write = {fs_op_write, 2},
    mkdir = {fs_op_mkdir, 1},
    remove = {fs_op_remove, 1},
    rename = {fs_op_rename, 2},
}

local function count_true(values)
    local cnt = 0
    for _, v in ipairs(values) do if v then cnt = cnt + 1 end end
    return cnt
end

local function cmd_fs(opts, args)
    cli.parse_opts(opts, {
        -- Source
        device = device_opt(nil),
        program = cli.str_opt(nil),
        image = cli.str_opt(nil),
        -- Destination
        output = cli.str_opt(nil),
        -- Options
        format = cli.bool_opt(false),
        block_size = cli.num_opt(4096),
        picotool = cli.str_opt('picotool'),
        -- TODO: --label, --reboot, --help
    })

    -- Check options.
    local srcs = count_true{opts.device, opts.program, opts.image}
    if srcs > 1 then
        raise("--device, --program and --image are mutually exclusive")
    end
    if opts.program and not opts.output then
        raise("--output is required with --program")
    end
    if opts.program then opts.format = true end

    -- Read the block device data from the source.
    printf("Reading filesystem source\n")
    local addr, size, data
    if opts.device or opts.program then
        addr, size = find_flash_drive(opts)
        if not (addr and size) then raise("no embedded drive found") end
    elseif opts.image then
        data, addr = read_uf2(opts.image)
        size = #data
    end
    if opts.format then
        if not data then data = mem.alloc(size) end
        mem.fill(data, 0xff)
    elseif opts.device then
        data = read_flash_range(opts, addr, size)
    end
    printf("Filesystem: 0x%08x bytes at 0x%08x\n", #data, addr)
    local orig = mem.read(data)

    -- Create the filesystem and mount it.
    local efs<close> = lfs.new(block_mem.new(data, 256, opts.block_size))
    if opts.format then
        local ok, msg = efs:format()
        if not ok then raise("failed to format filesystem: %s", msg) end
    end
    local ok, msg = efs:mount()
    if not ok then raise("failed to mount filesystem: %s", msg) end

    -- Perform the requested filesystem operations.
    local i = 1
    while i <= #args do
        local op = fs_ops[args[i]]
        if not op then raise("unknown filesystem operation: %s", args[i]) end
        i = i + 1
        local fn, nargs = table.unpack(op)
        if i + nargs > #args + 1 then
            raise("%s: not enough arguments", args[i])
        end
        fn(efs, table.unpack(args, i, i + nargs - 1))
        i = i + nargs
    end

    -- Unmount the filesystem and write the content of the block device to the
    -- destination if it was modified.
    ok, msg = efs:unmount()
    if not ok then raise("failed to unmount filesystem: %s", msg) end
    if mem.read(data) == orig then return end
    printf("Writing modified filesystem\n")
    local out = opts.output or opts.image or os.tmpname()
    local done<close> = function()
        if not (opts.output or opts.image) then os.remove(out) end
    end
    write_uf2(data, addr, out)
    if opts.device and not opts.output then write_flash_range(opts, out) end
end

local commands = {
    fs = cmd_fs,
}

function main() return cli.run(arg, commands) end
