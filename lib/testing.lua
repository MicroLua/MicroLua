-- A unit-testing library.

_ENV = require 'module'(...)

local eio = require 'eio'
local oo = require 'oo'
local package = require 'package'
local string = require 'string'
local table = require 'table'
local util = require 'util'

local def_mod_pat = '%.tests$'
local def_func_pat = '^test_'
local err_terminate = {}

Test = oo.class('Test')

function Test:__init(name, parent)
    self.name = name
    self.parent = parent
    if parent then parent.children[#parent.children + 1] = self end;
    self.errors = 0
    self.skipped = false
    self.children = {}
end

function Test:message(format, ...)
    if format:sub(-1) ~= '\n' then format = format .. '\n' end
    eio.write(string.format(format, ...))
    -- TODO: Include location
end

function Test:error(format, ...)
    self.errors = self.errors + 1
    self:message("ERROR: " .. format, ...)
end

function Test:fatal(format, ...)
    self:error(format, ...)
    error(err_terminate)
end

function Test:skip(format, ...)
    self.skipped = true
    self:message("SKIP: " .. format, ...)
    error(err_terminate)
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

function Test:result()
    return self:failed() and 'FAIL' or self.skipped and 'SKIP' or 'PASS'
end

function Test:run(name, func)
    local t = Test(name, self)
    local b = eio.Buffer()
    local old_out = eio.stdout
    eio.stdout = b
    local res, err = pcall(func, t)
    if not res then
        if err ~= err_terminate then t:error("%s", err) end
    end
    eio.stdout = old_out
    if t:failed() and not b:is_empty() then
        eio.write(string.format("----- BEGIN %s\n", name))
        b:replay(eio.stdout)
        eio.write(string.format("----- END   %s\n", name))
    end
end

function Test:run_all(module, pat)
    pat = pat or def_runc_pat
    local names = util.sort(util.keys(module,
                                      function(n) return n:find(pat) end))
    for _, name in ipairs(names) do
        self:run(name, module[name])
    end
end

function Test:print_result(indent, all)
    indent = indent or ''
    eio.write(string.format("%s%s: %s\n", indent, self:result(),
                           self.name))
    for _, t in ipairs(self.children) do
        if all or t:failed() then
            t:print_result(indent .. '  ', all)
        end
    end
end

function run_all_modules(t, mod_pat, func_pat)
    mod_pat = mod_pat or def_mod_pat
    func_pat = func_pat or def_func_pat
    local names = util.sort(util.keys(package.preload,
                                      function(n) return n:find(mod_pat) end))
    for _, name in ipairs(names) do
        t:run(name, function(t) t:run_all(require(name), func_pat) end)
    end
end

function run_tests_main()
    local t = Test()
    run_all_modules(t)
    eio.write("\n")
    for _, c in ipairs(t.children) do
        c:print_result()
    end
    eio.write(string.format("\nResult: %s\n\n", t:result()))
end
