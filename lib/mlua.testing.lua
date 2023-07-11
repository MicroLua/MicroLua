-- A unit-testing library.

_ENV = mlua.Module(...)

local debug = require 'debug'
local io = require 'mlua.io'
local oo = require 'mlua.oo'
local util = require 'mlua.util'
local math = require 'math'
local package = require 'package'
local time = require 'pico.time'
local string = require 'string'
local table = require 'table'

local def_mod_pat = '%.test$'
local def_func_pat = '^test_'
local err_terminate = {}

local module_name = ...

local Buffer = oo.class('Buffer', io.Buffer)

function Buffer:__init(t) self.t = t end

function Buffer:write(...)
    io.Buffer.write(self, ...)
    if not self:is_empty() and self.t:_attr('_full_output') then
        self.t:enable_output()
    end
end

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

function Test:run(name, fn)
    local t = Test(name, self)
    if t:_run(fn) then self.children = util.append(self.children, t) end
    t = nil
    collectgarbage('collect')
end

function Test:_run(fn)
    self:_capture_output()
    local start = time.get_absolute_time()
    self:_pcall(fn, self)
    self.duration = time.get_absolute_time() - start
    for i = util.len(self._cleanups), 1, -1 do
        self:_pcall(self._cleanups[i])
    end
    self._cleanups = nil
    self:_restore_output()
    if not self._out then io.printf("----- END   %s\n", self.name) end
    local keep = not self._parent._parent or self.children or self:failed()
    self:_walk_up(function(t)
        keep = keep or t._full_results
        if not t.failed_cnt then return end
        if self:failed() then t.failed_cnt = t.failed_cnt + 1
        elseif self._skipped then t.skipped_cnt = t.skipped_cnt + 1
        else t.passed_cnt = t.passed_cnt + 1 end
    end)
    self._parent, self._out = nil, nil
    return keep
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
    self._out = Buffer(self)
    self._old_out, _G.stdout = _G.stdout, self._out
end

function Test:_restore_output()
    if not self._old_out then return end
    _G.stdout, self._old_out = self._old_out, nil
end

function Test:_walk_up(fn)
    local t = self
    while t do
        local res = fn(t)
        if res ~= nil then return res end
        t = t._parent
    end
end

function Test:_attr(name)
    return self:_walk_up(function(t) return t[name] end)
end

local fn_comp = util.table_comp{1, 2}

function Test:run_module(name, pat)
    pat = pat or def_func_pat
    local module = require(name)
    self:cleanup(function() package.loaded[name] = nil end)
    local ok, fn = pcall(function() return module.set_up end)
    if ok and fn then fn(self) end
    local fns = {}
    for name, fn in pairs(module) do
        if name:find(pat) then
            local info = debug.getinfo(fn, 'S')
            util.append(fns, {info and info.linedefined or 0, name, fn})
        end
    end
    for _, fn in ipairs(util.sort(fns, fn_comp)) do self:run(fn[2], fn[3]) end
end

function Test:run_modules(mod_pat, func_pat)
    mod_pat = mod_pat or def_mod_pat
    func_pat = func_pat or def_func_pat
    for _, name in ipairs(util.sort(util.keys(package.preload))) do
        if name:find(mod_pat) then
            self:run(name, function(t) t:run_module(name, func_pat) end)
        end
    end
end

function Test:main(runs)
    io.printf("Running tests\n")
    local start = time.get_absolute_time()
    if runs > 1 then
        for i = 1, runs do
            self:run(("Run #%s"):format(i), function(t) t:run_modules() end)
        end
    else
        self:run_modules()
    end
    local dt = time.get_absolute_time() - start
    local mem = collectgarbage('count') * 1024
    io.write("\n")
    self:print_result()
    io.printf("\nTests: %d passed, %d skipped, %d failed, %d total\n",
               self.passed_cnt, self.skipped_cnt, self.failed_cnt,
               self.passed_cnt + self.skipped_cnt + self.failed_cnt)
    io.printf("Duration: %.3f s, memory: %d bytes\n", dt / 1e6, mem)
    io.printf("Result: %s\n\n", self:result())
end

function Test:print_result(indent)
    indent = indent or ''
    if self.name then
        local left = ('%s%s: %s'):format(indent, self:result(), self.name)
        local right = ('(%.3f s)'):format(self.duration / 1e6)
        io.printf("%s%s %s\n", left, (' '):rep(78 - #left - #right), right)
        indent = indent .. '  '
    end
    if not self.children then return end
    for _, t in ipairs(self.children) do t:print_result(indent) end
end

function pmain()
    local full_output, full_results = false, false
    local runs = 1
    while true do
        local t = Test()
        t._full_output = full_output
        t._full_results = full_results
        t:main(runs)

        io.printf("(testing) ")
        local line = ''
        while true do
            local c = stdin:read(1)
            if c == '\n' then break end
            line = line .. c
        end
        if line == 'fo' then full_output = not full_output
        elseif line == 'fr' then full_results = not full_results
        elseif line == 'q' then break
        elseif line:sub(1, 1) == 'r' then
            runs = math.tointeger(tonumber(line:sub(2))) or 1
        end
    end
end

function main()
    ok, err = pcall(pmain)
    if not ok then io.printf("ERROR: %s\n", err) end
end
