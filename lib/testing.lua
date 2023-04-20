-- A unit-testing library.

_ENV = require 'module'(...)

local eio = require 'eio'
local oo = require 'oo'
local os = require 'os'
local package = require 'package'
local string = require 'string'
local table = require 'table'

local def_mod_pat = '%.tests$'
local def_func_pat = '^test_'
local err_terminate = {}

Test = oo.class('Test')

function Test:__init(name, parent)
    self.name, self._parent = name, parent
end

function Test:message(format, ...)
    if format:sub(-1) ~= '\n' then format = format .. '\n' end
    eio.printf(format, ...)
    -- TODO: Include location
end

function Test:error(format, ...)
    self._failed = true
    self:_enable_output()
    self:message("ERROR: " .. format, ...)
end

function Test:fatal(format, ...)
    self:error(format, ...)
    error(err_terminate)
end

function Test:skip(format, ...)
    self._skipped = true
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
    if self._failed then return true end
    if self.children then
        for _, child in ipairs(self.children) do
            if child:failed() then return true end
        end
    end
    return false
end

function Test:result()
    return self:failed() and 'FAIL' or self._skipped and 'SKIP' or 'PASS'
end

function Test:run(name, func, keep)
    local t = Test(name, self)
    t:_run(func)
    if keep or t.children or t:failed() then
        if not self.children then self.children = {} end
        self.children[#self.children + 1] = t
    end
    t = nil
    collectgarbage('collect')
end

function Test:_run(func)
    self:_capture_output()
    local res, err = pcall(func, self)
    if not res then
        if err ~= err_terminate then self:error("%s", err) end
    end
    self:_restore_output()
    if not self._out then
        stdout:write(string.format("----- END   %s\n", self.name))
    end
    self._parent, self._out = nil, nil
end

function Test:_enable_output()
    if not self._out then return end
    local out = self._out
    self._out = nil
    self:_restore_output()
    if self._parent then self._parent:_enable_output() end
    stdout:write(string.format("----- BEGIN %s\n", self.name))
    out:replay(stdout)
end

function Test:_capture_output()
    self._out = eio.Buffer()
    self._old_out, _G.stdout = _G.stdout, self._out
end

function Test:_restore_output()
    if not self._old_out then return end
    _G.stdout, self._old_out = self._old_out, nil
end

function Test:run_all(module, pat)
    pat = pat or def_func_pat
    for name, func in pairs(module) do
        if name:find(pat) then
            self:run(name, func)
        end
    end
end

function Test:run_all_modules(mod_pat, func_pat)
    mod_pat = mod_pat or def_mod_pat
    func_pat = func_pat or def_func_pat
    for name in pairs(package.preload) do
        if name:find(mod_pat) then
            self:run(name, function(t)
                t:run_all(require(name), func_pat)
            end, true)
        end
    end
end

function Test:print_result(indent)
    indent = indent or ''
    if self.name then
        eio.printf("%s%s: %s\n", indent, self:result(), self.name)
        indent = indent .. '  '
    end
    if not self.children then return end
    table.sort(self.children, function(a, b) return a.name < b.name end)
    for _, t in ipairs(self.children) do
        t:print_result(indent)
    end
end

function run_tests_main()
    local t = Test()
    local start = os.clock()
    t:run_all_modules()
    local dt = os.clock() - start
    local mem = collectgarbage('count') * 1024
    eio.write("\n")
    t:print_result()
    eio.printf("\nCPU time: %.2f s\n", dt)
    eio.printf("Memory: %d bytes\n", mem)
    eio.printf("Result: %s\n\n", t:result())
end
