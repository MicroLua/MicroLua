-- A unit-testing library.

_ENV = mlua.Module(...)

local debug = require 'debug'
local io = require 'mlua.io'
local oo = require 'mlua.oo'
local util = require 'mlua.util'
local os = require 'os'
local package = require 'package'
local string = require 'string'
local table = require 'table'

local def_mod_pat = '%.test$'
local def_func_pat = '^test_'
local err_terminate = {}

local module_name = ...

Test = oo.class('Test')
Test.helper = 'xGJLirXXSePWnLIqptqM85UBIpZdaYs86P7zfF9sWaa3'

function Test:__init(name, parent)
    self.name, self._parent = name, parent
    if not parent then
        self.failed_cnt, self.skipped_cnt, self.passed_cnt = 0, 0, 0
    end
end

function Test:cleanup(fn) self._cleanups = util.append(self._cleanups, fn) end

function Test:patch(tab, name, value)
    local old = rawget(tab, name)
    self:cleanup(function() rawset(tab, name, old) end)
    rawset(tab, name, value)
end

function Test:printf(format, ...) return io.printf(format, ...) end

function Test:log(format, ...) return self:_log('', format, ...) end

function Test:error(format, ...)
    self._failed = true
    self:enable_output()
    return self:_log("ERROR: ", format, ...)
end

function Test:fatal(format, ...)
    self:error(format, ...)
    error(err_terminate)
end

function Test:skip(format, ...)
    self._skipped = true
    self:_log("SKIP: ", format, ...)
    error(err_terminate)
end

function Test:expect(cond, format, ...)
    if not cond then return self:error(format, ...) end
end

function Test:assert(cond, format, ...)
    if not cond then return self:fatal(format, ...) end
end

function Test:_log(prefix, format, ...)
    local msg = format:format(...)
    local eol = msg:sub(-1) ~= '\n' and '\n' or ''
    return io.printf('%s%s%s%s', prefix, self:_location(1), msg, eol)
end

function Test:_location(level)
    level = level + 2
    while true do
        local info = debug.getinfo(level, 'Sl')
        if not info then return '' end
        local src, line = info.short_src, info.currentline
        if src ~= module_name and line > 0 then
            local i = 1
            while true do
                local name, value = debug.getlocal(level, i)
                if not name then return ('%s:%s: '):format(src, line) end
                if value == self.helper then break end
                i = i + 1
            end
        end
        level = level + 1
    end
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

function Test:run(name, fn, keep)
    local t = Test(name, self)
    t:_run(fn)
    if keep or t.children or t:failed() then
        self.children = util.append(self.children, t)
    end
    t = nil
    collectgarbage('collect')
end

function Test:_root()
    local t = self
    while true do
        local p = t._parent
        if not p then return t end
        t = p
    end
end

function Test:_run(fn)
    self:_capture_output()
    self:_pcall(fn, self)
    for i = util.len(self._cleanups), 1, -1 do
        self:_pcall(self._cleanups[i])
    end
    self._cleanups = nil
    self:_restore_output()
    if not self._out then io.printf("----- END   %s\n", self.name) end
    local root = self:_root()
    if self:failed() then root.failed_cnt = root.failed_cnt + 1
    elseif self._skipped then root.skipped_cnt = root.skipped_cnt + 1
    else root.passed_cnt = root.passed_cnt + 1 end
    self._parent, self._out = nil, nil
end

function Test:_pcall(fn, ...)
    local ok, err = pcall(fn, ...)
    if not ok and err ~= err_terminate then return self:error("%s", err) end
end

function Test:enable_output()
    if not self._out then return end
    local out = self._out
    self._out = nil
    self:_restore_output()
    if self._parent then self._parent:enable_output() end
    io.printf("----- BEGIN %s\n", self.name)
    out:replay(stdout)
end

function Test:_capture_output()
    self._out = io.Buffer()
    self._old_out, _G.stdout = _G.stdout, self._out
end

function Test:_restore_output()
    if not self._old_out then return end
    _G.stdout, self._old_out = self._old_out, nil
end

function Test:run_module(name, pat)
    pat = pat or def_func_pat
    local module = require(name)
    self:cleanup(function() package.loaded[name] = nil end)
    local ok, fn = pcall(function() return module.set_up end)
    if ok and fn then fn(self) end
    for name, fn in pairs(module) do
        if name:find(pat) then self:run(name, fn) end
    end
end

function Test:run_all_modules(mod_pat, func_pat)
    mod_pat = mod_pat or def_mod_pat
    func_pat = func_pat or def_func_pat
    for name in pairs(package.preload) do
        if name:find(mod_pat) then
            self:run(name, function(t) t:run_module(name, func_pat) end, true)
        end
    end
end

function Test:print_result(indent)
    indent = indent or ''
    if self.name then
        io.printf("%s%s: %s\n", indent, self:result(), self.name)
        indent = indent .. '  '
    end
    if not self.children then return end
    table.sort(self.children, function(a, b) return a.name < b.name end)
    for _, t in ipairs(self.children) do t:print_result(indent) end
end

function main()
    local t = Test()
    io.printf("Running tests\n")
    local start = os.clock()
    t:run_all_modules()
    local dt = os.clock() - start
    local mem = collectgarbage('count') * 1024
    io.write("\n")
    t:print_result()
    io.printf("\nTests: %d passed, %d skipped, %d failed, %d total\n",
               t.passed_cnt, t.skipped_cnt, t.failed_cnt,
               t.passed_cnt + t.skipped_cnt + t.failed_cnt)
    io.printf("CPU time: %.2f s, memory: %d bytes\n", dt, mem)
    io.printf("Result: %s\n\n", t:result())
end
