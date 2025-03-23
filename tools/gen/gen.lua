-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

-- Perform code generation build steps. The following sub-commands are
-- available:
--
--  - cmod: Pre-process a C module source file.
--  - configmod: Generate a C module providing symbols defined in the build
--    system.
--  - headermod: Generate a C module providing the preprocessor symbols defined
--    by a header file.
--  - luamod: Generate a C module from a Lua source file.

local io = require 'io'
local os = require 'os'
local string = require 'string'
local table = require 'table'

-- Raise an error without location information.
local function raise(format, ...) return error(format:format(...), 0) end

-- Raise an error if the first argument is false, otherwise return all
-- arguments.
local function assert(...)
    local ok, msg = ...
    if not ok then return error(msg, 0) end
    return ...
end

-- Print to stdout.
local function printf(format, ...) return stdout:write(format:format(...)) end

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

-- Convert a list of values to a list of strings.
local function to_strings(values)
    local res = {}
    for _, v in ipairs(values) do table.insert(res, tostring(v)) end
    return res
end

-- Read a file and return its content.
local function read_file(path)
    local f<close> = assert(io.open(path, 'r'))
    return f:read('a')
end

-- Write a file atomically.
local function write_file(path, data)
    local tmp = path .. '.tmp'
    local commit<close> = function(_, err)
        local ok = not err
        if ok then ok, err = os.rename(tmp, path) end
        if not ok then
            os.remove(tmp)
            error(err, 0)
        end
    end
    local f<close> = assert(io.open(tmp, 'w'))
    assert(f:write(data))
end

-- Iterate over the lines of the given string.
local function lines(text) return text:gmatch('[^\n]*\n?') end

-- Parse cmake-style keyword arguments.
local function parse_kwargs(keys, args)
    local kwargs = {}
    for _, k in ipairs(keys) do kwargs[k] = {} end
    local cur
    for _, a in ipairs(args) do
        local as = kwargs[a]
        if as then cur = as
        elseif cur then table.insert(cur, a)
        else raise("missing keyword argument") end
    end
    return kwargs
end

local Graph = {}
Graph.__index = Graph

-- Create a new graph with the given number of vertices.
local function new_graph(nv, ne)
    return setmetatable({nv = nv, ne = ne, adj = {}}, Graph)
end

-- Connect the given vertices, and assign the edge value.
function Graph:connect(v1, v2, ev)
    if not self.adj[v1] then self.adj[v1] = {} end
    table.insert(self.adj[v1], {v2, ev})
    if not self.adj[v2] then self.adj[v2] = {} end
    table.insert(self.adj[v2], {v1, ev})
end

-- Simultaneously perform the assignment step and the check for a cyclic graph.
-- We're already traversing the whole graph, so we might as well do the check at
-- the same time.
function Graph:assign()
    self.g = {}  -- Vertex values
    local visited = {}

    -- Traverse the graph starting from each vertex.
    for root = 1, self.nv do
        -- Check if we've already visited this connected component.
        if visited[root] then goto next_root end

        -- Assign an arbitrary value to the root vertex.
        self.g[root] = 0

        -- Traverse the connected component starting from the root, by keeping
        -- a stack of vertices to visit and their parent.
        local stack = {{nil, root}}
        while stack[1] do
            local parent, vertex = table.unpack(table.remove(stack), 1, 2)
            visited[vertex] = true

            -- Compute the values of adjacent vertices, and push them onto the
            -- stack.
            local parent_seen = false
            for _, a in ipairs(self.adj[vertex] or {}) do
                local neigh, ev = table.unpack(a)

                -- Ignore one edge to the parent: the graph is undirected, so
                -- there's always at least one.
                if not parent_seen and neigh == parent then
                    parent_seen = true
                    goto next_adj
                end

                -- If we reach an already-visited vertex, the graph is cyclic.
                if visited[neigh] then return false end

                -- Set the neighboring vertex value.
                self.g[neigh] = (ev - self.g[vertex]) % self.ne

                -- Push the vertex onto the stack as a next hop.
                table.insert(stack, {vertex, neigh})
                ::next_adj::
            end
        end
        ::next_root::
    end
    return true
end

-- FNV-1a defines 0x01000193 as the hash multiplier for 32 bits, but this works
-- just as well and is faster to compute (fewer 1 bits).
local hash_mult = 0x13

-- Compute an FNV-1a style hash of the given key, with a specific seed value.
local function hash(key, seed)
    for i = 1, #key do
        seed = (seed ~ key:byte(i)) * hash_mult
    end
    return seed & 0x7fffffff  -- Avoid negative numbers
end

-- Compute the perfect hash of a key with a set of parameters.
local function perfect_hash(key, h)
    return (h.g[hash(key, h.seed1) % #h.g + 1]
            + h.g[hash(key, h.seed2) % #h.g + 1]) % h.nkeys
end

-- Perform the mapping step for the given keys. Returns the hash parameters.
--
-- This function implements the CHM92 algorithm, as described in:
-- "An optimal algorithm for generating minimal perfect hash functions",
-- Z. J. Czech, G. Havas and B. S. Majewski,
-- Information Processing Letters, 43(5):257-264, 1992
-- https://citeseerx.ist.psu.edu/doc/10.1.1.51.5566
local function find_perfect_hash(keys)
    local nk = #keys  -- Number of keys
    -- Considering nv = c * nk:
    --  - c <= 1: Finding an acyclic graph is impossible.
    --  - c <= 2: The probability of finding an acyclic graph tends towards
    --            zero as the number of keys tends towards infinity.
    --  - c > 2: The probability tends towards sqrt((c - 2) / 2).
    -- For small key counts, we start at the minimum number of vertices and
    -- increase if too many attempts are needed. And for large key counts, we
    -- use the lowest number for which the probability is finite.
    local nvmax = 2 * nk + 1
    local nv = nk < 100 and nk + 1 or nvmax  -- Number of vertices
    local seed1, seed2 = 1, 2  -- Hash seeds

    -- Generate graphs from hash pairs until the assignment step succeeds, i.e.
    -- the graph is acyclic.
    local h, attempt = nil, 1
    while true do
        if attempt % 100 == 0 and nv < nvmax then nv = nv + 1 end
        local g = new_graph(nv, nk)
        for i, key in ipairs(keys) do
            local v1, v2 = hash(key, seed1) % nv + 1, hash(key, seed2) % nv + 1
            if v1 == v2 then goto next_attempt end  -- Graph is trivially cyclic
            g:connect(v1, v2, i - 1)
        end
        if g:assign() then
            h = {seed1 = seed1, seed2 = seed2, nkeys = #keys, g = g.g}
            break
        end
        ::next_attempt::
        seed1 = seed1 + 7
        seed2 = seed2 + 13
        attempt = attempt + 1
    end

    -- Check the consistency of the hash.
    for i, key in ipairs(keys) do
        local kh = perfect_hash(key, h)
        if kh ~= i - 1 then
            raise("bad hash: %s -> %s, want %s", key, kh, i - 1)
        end
    end
    return h
end

-- Return the number of bits necessary to represent key indices for the given
-- number of keys.
local function key_bits(nkeys)
    for i = 0, 32 do
        if nkeys <= 1 << i then return i end
    end
end

-- Pack the vertex values of a hash as a byte array, using the minimum number of
-- bits per value.
local function pack_hash(h)
    local bits = key_bits(h.nkeys)
    local data = {}
    if bits == 0 then return data, bits end
    for i = 1, (#h.g * bits + 7) // 8 do data[i] = 0 end
    for i, v in ipairs(h.g) do
        local bi = (i - 1) * bits
        v = v << bi % 8
        local index = bi // 8 + 1
        while v ~= 0 do
            data[index] = data[index] | (v & 0xff)
            v = v >> 8
            index = index + 1
        end
    end
    return data, bits
end

-- Check that the packed vertex values unpack to the original values.
local function check_packed_hash(h, data)
    local bits = key_bits(h.nkeys)
    local mask = (1 << bits) - 1
    for i, gv in ipairs(h.g) do
        local value = 0
        if bits > 0 then
            local bi = (i - 1) * bits
            local index, shift = bi // 8 + 1, bi % 8
            value = data[index]
            local be = bits + shift
            if be > 8 then
                value = value | (data[index + 1] << 8)
                if be > 16 then value = value | (data[index + 2] << 16) end
            end
            value = (value >> shift) & mask
        end
        if value ~= gv then
            raise("packed hash inconsistency: index=%s, value=%s, gv=%s", i - 1,
                  value, gv)
        end
    end
end

-- Format a perfect hash for embedding into a C file.
local function format_hash(suffix, name, h, out)
    local data, bits = pack_hash(h)
    check_packed_hash(h, data)
    table.insert(out,
        ('}; MLUA_SYMBOLS_HASH_FN%s(%s, %s, %s, %s, %s, %s, '):format(
            suffix, name, h.seed1, h.seed2, h.nkeys, bits, #h.g))
    for i = 0, #data - 1 do
        table.insert(out, ('0x%02x,'):format(data[i + 1] or 0))
    end
    table.insert(out, ')\n')
end

local max_syms = 1 << 16

-- Preprocess C a module source.
local function preprocess_cmod(text)
    local out, suffix, name, syms, seen = {}
    for line in lines(text) do
        table.insert(out, line)
        if syms then
            local sym = line:match(
                '^%s*MLUA_SYM_[%u%d_]+%(%s*([%a_][%w_]*)%s*,')
            if sym then
                if sym:match('^_[^_]') then sym = sym:sub(2) end
                if not seen[sym] then
                    table.insert(syms, sym)
                    seen[sym] = true
                end
            elseif line:match('^%s*}%s*;%s*$') then
                if #syms > max_syms then
                    raise("too many symbols: got %s, max %s", #syms, max_syms)
                end
                local h = find_perfect_hash(syms)
                table.remove(out)
                format_hash(suffix, name, h, out)
                suffix, name, syms, seen = nil
            end
        else
            local s, n = line:match(
                '^%s*MLUA_SYMBOLS([%u_]*)%(%s*([%a_][%w_]*)%s*%)%s*=%s*{%s*$')
            if s == '' or s == '_HASH' then
                suffix, name, syms, seen = s, n, {}, {}
            end
        end
    end
    return table.concat(out)
end

-- Preprocess a C module source file.
function cmd_cmod(args)
    local input, output = table.unpack(args, 1, 2)
    write_file(output, preprocess_cmod(read_file(input)))
end

-- Generate a C module providing symbols defined in the build system.
function cmd_configmod(args)
    local mod, template, output = table.unpack(args, 1, 3)
    local syms = {}
    for _, sym in ipairs(slice(args, 4)) do
        local name, typ, value = sym:match('^([^:]+):([^=]+)=(.*)$')
        if not name then raise("invalid symbol definition: %s", sym) end
        table.insert(syms,
            ('    MLUA_SYM_V_H(%s, %s, %s),'):format(name, typ, value))
    end
    local tmpl = read_file(template)
    local sub = {MOD = mod, SYMBOLS = table.concat(syms, '\n')}
    write_file(output, preprocess_cmod(tmpl:gsub('@(%u+)@', sub)))
end

-- Parse type mapping.
local function typemap(types)
    local map = {}
    for _, it in ipairs(types) do
        local pat, v, cast = it:match('^([^=]*)=([^:]+):(.*)$')
        if not pat then pat, v = it:match('^([^=]*)=(.*)$') end
        if not pat then raise("invalid type definition: %s", it) end
        table.insert(map, {pat, v, cast or ''})
    end
    table.insert(map, {':"', 'string', ''})
    table.insert(map, {'', 'integer', ''})
    return map
end

-- Parse preprocessor symbols that define values and enum values.
local function parse_symbols(text, excludes, strip, types)
    local syms = {}
    for line in lines(text) do
        local pp = true
        local csym, val = line:match(
            '^#define%s+([a-zA-Z][a-zA-Z0-9_]*)%s+(.+)\n$')
        if not csym then
            pp = false
            csym, val = line:match(
                '^%s*([a-zA-Z][a-zA-Z0-9_]*)%s*=%s*(.+)%s*,%s*\n$')
        end
        if not csym then
            pp = false
            csym, val = line:match('^%s*([a-zA-Z][a-zA-Z0-9_]*)%s*,?%s*\n$'), ''
        end
        if not csym then goto continue end
        local sv = ('%s:%s'):format(csym, val)
        for _, exclude in ipairs(excludes) do
            if sv:match(exclude) then goto continue end
        end
        local sym = csym
        for _, strip in ipairs(strip) do
            local s, e, cs, ce = sym:find(strip)
            if cs and ce then s, e = cs, ce end
            if s and e then
                sym = sym:sub(1, s - 1) .. sym:sub(e + 1)
                break
            end
        end
        for _, it in ipairs(types) do
            local pat, typ, cast = table.unpack(it)
            if sv:match(pat) then
                syms[sym] = {typ, cast, csym, pp}
                break
            end
        end
        ::continue::
    end
    return syms
end

-- Generate a C module providing the preprocessor and enum symbols defined by a
-- header file.
function cmd_headermod(args)
    local mod, include, defines, template, output = table.unpack(args, 1, 5)
    local kwargs = parse_kwargs({'EXCLUDE', 'STRIP', 'TYPES'}, slice(args, 6))
    local syms = parse_symbols(read_file(defines), kwargs.EXCLUDE, kwargs.STRIP,
                               typemap(kwargs.TYPES))
    local names = {}
    for sym in pairs(syms) do table.insert(names, sym) end
    table.sort(names)
    local symdefs = {}
    for _, name in ipairs(names) do
        local typ, cast, csym, pp = table.unpack(syms[name])
        if pp then table.insert(symdefs, ('#ifdef %s'):format(csym)) end
        table.insert(symdefs,
            ('    MLUA_SYM_V_H(%s, %s, %s%s),'):format(name, typ, cast, csym))
        if pp then
            table.insert(symdefs, '#else')
            table.insert(symdefs,
                ('    MLUA_SYM_V_H(%s, boolean, false),'):format(name))
            table.insert(symdefs, '#endif')
        end
    end
    local tmpl = read_file(template)
    local sub = {
        MOD = mod, INCLUDE = include, SYMBOLS = table.concat(symdefs, '\n'),
    }
    write_file(output, preprocess_cmod(tmpl:gsub('@(%u+)@', sub)))
end

-- Compile a Lua module and return the generated chunk as C array data.
local function compile_lua(mod, src)
    -- Compile the input file.
    local chunk = assert(load(src, '@' .. mod))
    local bin = string.dump(chunk)

    -- Format the compiled chunk as C array data.
    local out = {}
    for i = 1, #bin do table.insert(out, ('0x%02x,'):format(bin:byte(i))) end
    return table.concat(out)
end

-- Generate a C module from a Lua source file.
function cmd_luamod(args)
    local mod, src, template, output = table.unpack(args, 1, 4)
    local data = compile_lua(mod, read_file(src))
    local tmpl = read_file(template)
    local sub = {MOD = mod, DATA = data, INCBIN = '0'}
    write_file(output, tmpl:gsub('@(%u+)@', sub))
end

-- Dispatch to the selected sub-command.
function main()
    local cmd = arg[1]
    local fn = _ENV['cmd_' .. cmd]
    if not fn then raise("unknown command: %s", cmd) end
    return fn(slice(arg, 2))
end
