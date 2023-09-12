-- Perform code generation build steps. The following sub-commands are
-- available:
--
--  - cmod: Pre-process a C module source file.
--  - coremod: Generate a C file that registers a core C module.
--  - luamod: Generate a C module from a Lua source file.

local io = require 'io'
local os = require 'os'
local string = require 'string'
local table = require 'table'

-- Raise an error without position information.
local function raise(format, format, ...)
    return error(format:format(...), 0)
end

-- Raise an error if the first argument is false, otherwise return all
-- arguments.
local function assert(...)
    local ok, msg = ...
    if not ok then return error(msg, 0) end
    return ...
end

-- Print to stdout.
local function printf(format, ...) io.stdout:write(format:format(...)) end

-- Return a table that calls the given function when it is closed.
local function defer(fn) return setmetatable({}, {__close = fn}) end

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
    local commit<close> = defer(function(_, err)
        local ok = not err
        if ok then ok, err = os.rename(tmp, path) end
        if not ok then
            os.remove(tmp)
            error(err, 0)
        end
    end)
    local f<close> = assert(io.open(tmp, 'w'))
    assert(f:write(data))
end

-- Iterate over the lines of the given string.
local function lines(text) return text:gmatch('[^\n]*\n?') end

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
            local parent, vertex = table.unpack(table.remove(stack))
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

local hash_mult = 0x01000193

-- Compute an FNV-1a hash of the given key, with a specific seed value.
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
    local nv = nk + 1  -- Number of vertices
    local seed1, seed2 = 1, 2  -- Hash seeds

    -- Generate graphs from hash pairs until the assignment step succeeds, i.e.
    -- the graph is acyclic. We start with the minimum number of vertices, and
    -- increase it if too many attempts are required.
    local h, attempt = nil, 1
    while true do
        if attempt % 100 == 0 then nv = nv + 1 end
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
            error(("Bad hash: %s -> %s, want %s"):format(key, kh, i - 1))
        end
    end
    return h
end

-- Preprocess C a module source.
local function preprocess_cmod(text)
    local out, name, syms, seen = {}
    for line in lines(text) do
        table.insert(out, line)
        if syms then
            local sym = line:match('^%s*MLUA_SYM_[%u%d_]+%(%s*([%a_][%w_]*)%s*,')
            if sym then
                if sym:match('^_[^_]') then sym = sym:sub(2) end
                if not seen[sym] then
                    table.insert(syms, sym)
                    seen[sym] = true
                end
            elseif line:match('^%s*}%s*;%s*$') then
                local h = find_perfect_hash(syms)
                table.insert(out,
                    ('MLUA_SYMBOLS_HASH(%s, %s, %s, %s, %s);\n'):format(
                        name, h.seed1, h.seed2, #syms, #h.g))
                table.insert(out,
                    ('MLUA_SYMBOLS_HASH_VALUES(%s) = {%s};\n'):format(
                        name, table.concat(to_strings(h.g), ",")))
                name, syms, seen = nil, nil, nil
            end
        else
            local n = line:match(
                '^%s*MLUA_SYMBOLS%(%s*([%a_][%w_]*)%s*%)%s*=%s*{%s*$')
            if n then name, syms, seen = n, {}, {} end
        end
    end
    return table.concat(out)
end

-- Preprocess C a module source file.
function cmd_cmod(input, output)
    write_file(output, preprocess_cmod(read_file(input)))
end

function cmd_configmod(mod, template, output, ...)
    local syms = {}
    for _, sym in ipairs(table.pack(...)) do
        local name, typ, value = sym:match('^([^:]+):([^=]+)=(.*)$')
        if not name then raise("invalid symbol definition: %s", sym) end
        table.insert(syms,
            ('    MLUA_SYM_V(%s, %s, %s),'):format(name, typ, value))
    end
    local tmpl = read_file(template)
    local sub = {MOD = mod, SYMBOLS = table.concat(syms, '\n')}
    write_file(output, tmpl:gsub('@(%u+)@', sub))
end

-- Generate a C file that registers a core C module.
function cmd_coremod(mod, template, output)
    local tmpl = read_file(template)
    local sub = {MOD = mod, SYM = mod:gsub('%.', '_')}
    write_file(output, tmpl:gsub('@(%u+)@', sub))
end

-- Compile a Lua module and return the generated chunk as C array data.
local function compile_lua(mod, src)
    -- Compile the input file.
    local chunk = assert(load(src, '@' .. mod))
    local bin = string.dump(chunk)

    -- Format the compiled chunk as C array data.
    local parts, offset = {}, 0
    while true do
        if offset >= #bin then break end
        for i = 1, 16 do
            local v = bin:byte(offset + i)
            if v == nil then break end
            table.insert(parts, ('0x%02x,'):format(v))
        end
        table.insert(parts, '\n')
        offset = offset + 16
    end

    -- Lua seems to rely on a terminating zero byte when loading chunks in text
    -- mode, even if it isn't included in the chunk size. This is likely a bug.
    -- We work around it by appending a zero byte to the output.
    table.insert(parts, '0x00,')
    return table.concat(parts)
end

-- Generate a C module from a Lua source file.
function cmd_luamod(mod, src, template, output)
    local data = compile_lua(mod, read_file(src))
    local tmpl = read_file(template)
    local sub = {MOD = mod, DATA = data}
    write_file(output, tmpl:gsub('@(%u+)@', sub))
end

-- Dispatch to the selected sub-command.
local function main(cmd, ...)
    local fn = _ENV['cmd_' .. cmd]
    if not fn then error(("unknown command: %s"):format(cmd), 0) end
    return fn(...)
end

local ok, err = pcall(main, ...)
if not ok then
    io.stderr:write(("ERROR: %s\n"):format(err))
    os.exit(false, true)
end
