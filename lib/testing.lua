_ENV = require 'module'(...)

local io = require 'io'
local oo = require 'oo'
local package = require 'package'
local string = require 'string'
local table = require 'table'

local def_mod_pat = '%.tests$'
local def_func_pat = '^test_'

local function result(failed)
    return failed and 'FAIL' or 'PASS'
end

local function sorted_keys(tab, filter)
    local res = {}
    for key in next, tab do
        if not filter or filter(key) then
            res[#res + 1] = key
        end
    end
    table.sort(res)
    return res
end

Test = oo.class('Test')

function Test:__init(name, parent)
    self.name = name
    self.parent = parent
    if parent then parent.children[#parent.children + 1] = self end;
    self.errors = 0
    self.children = {}
end

function Test:error(format, ...)
    self.errors = self.errors + 1
    if format:sub(-1) ~= '\n' then format = format .. '\n' end
    io.write(string.format("ERROR: " .. format, ...))
end

function Test:fatal(format, ...)
    self:error(format, ...)
    error("test failed", 2)
end

function Test:expect(cond, format, ...)
    if not cond then self:error(format, ...) end
end

function Test:assert(cond, format, ...)
    if not cond then self:fatal(format, ...) end
end

function Test:failed()
    if self.errors > 0 then return true end
    for _, child in ipairs(self.children) do
        if child:failed() then return true end
    end
    return false
end

function Test:run(name, func)
    local t = Test(name, self)
    io.write(string.format("--- BEGIN %s ---\n", name))
    -- TODO: pcall
    func(t)
    io.write(string.format("--- END %s ---\n", name))
end

function Test:run_all(module, pat)
    pat = pat or def_runc_pat
    local names = sorted_keys(module, function(n) return n:find(pat) end)
    for _, name in ipairs(names) do
        self:run(name, module[name])
    end
end

function Test:print_result(indent)
    indent = indent or ''
    io.write(string.format("%s%s: %s\n", indent, result(self:failed()),
                           self.name))
    for _, t in ipairs(self.children) do
        t:print_result(indent .. '  ')
    end
end

function run_all_modules(t, mod_pat, func_pat)
    mod_pat = mod_pat or def_mod_pat
    func_pat = func_pat or def_func_pat
    local names = sorted_keys(package.preload,
                              function(n) return n:find(mod_pat) end)
    for _, name in ipairs(names) do
        t:run(name, function(t) t:run_all(require(name), func_pat) end)
    end
end

function run_tests_main()
    local t = Test("All tests")
    run_all_modules(t)
    io.write("\n")
    for _, c in ipairs(t.children) do
        c:print_result()
    end
    io.write(string.format("\nResult: %s\n\n", result(t:failed())))
end
