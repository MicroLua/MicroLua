-- A unit-testing library.

_ENV = mlua.Module(...)

local debug = require 'debug'
local io = require 'mlua.io'
local list = require 'mlua.list'
local oo = require 'mlua.oo'
local util = require 'mlua.util'
local math = require 'math'
local package = require 'package'
local time = require 'pico.time'
local string = require 'string'
local table = require 'table'

local def_mod_pat = '%.test$'
local def_func_pat = '^test_'
local yield_pat = '_Y$'
local err_terminate = {}

local module_name = ...

local function pcall_res(ok, ...)
    if ok then return ok, list.pack(...) end
    return ok, (...)
end

-- Wrap the given function to print a traceback if it raises an error.
function with_traceback(fn)
    return function(...)
        local ok, res = pcall_res(xpcall(fn, function(err)
            io.printf("ERROR: %s\n", debug.traceback(err, 2))
            return err
        end, ...))
        if ok then return res:unpack() else error(res, 0) end
    end
end

local Buffer = oo.class('Buffer', io.Buffer)

function Buffer:__init(t) self.t = t end

function Buffer:write(...)
    io.Buffer.write(self, ...)
    if not self:is_empty() and self.t:_attr('_full_output') then
        self.t:enable_output()
    end
end

local function locals(level)
    level = (level or 1) + 1
    local loc, i = {}, 1
    while true do
        local name, value = debug.getlocal(level, i)
        if not name then break end
        if name:sub(1, 1) ~= '(' then loc[name] = value end
        i = i + 1
    end
    local info = debug.getinfo(level, 'f')
    if info then
        local f = info.func
        setmetatable(loc, {__index = function(_, k)
            local env, i = nil, 1
            while true do
                local name, value = debug.getupvalue(f, i)
                if not name then break end
                if name == k then return value end
                if name == '_ENV' then env = value end
                i = i + 1
            end
            if env then return env[k] end
        end})
    end
    return loc
end

local Expr = {__name = 'Expr'}
local ekey = {}

function Expr:__index(k)
    list.append(rawget(self, ekey), {
        function(s)
            if type(k) == 'string' and k:find('^[%a_][%w_]*$') then
                return ('%s%s%s'):format(s, s ~= '' and '.' or '', k)
            end
            return ('%s[%s]'):format(s, util.repr(k))
        end,
        function(pv, v) return v[k] end,
    })
    return self
end

function Expr:__call(...)
    -- TODO: Add support for calls returning multiple values
    local args = list.pack(...)
    list.append(rawget(self, ekey), {
        function(s)
            local parts, n = list(), 1
            if rawequal(args[1], self) then
                s = s:gsub('%.([%a_][%w_]*)$', ':%1')
                n = 2
            end
            for i = n, args:len() do parts:append(util.repr(args[i])) end
            return ('%s(%s)'):format(s, table.concat(parts, ', '))
        end,
        function(pv, v)
            if rawequal(args[1], self) then return v(pv, args:unpack(2)) end
            return v(args:unpack())
        end,
    })
    return self
end

function Expr:__repr(repr)
    local e = rawget(self, ekey)
    local s = ''
    for _, it in list.ipairs(e) do s = it[1](s) end
    return s
end

function Expr:__eval()
    local e = rawget(self, ekey)
    local pv, v = nil, e.r
    for _, it in list.ipairs(e) do pv, v = v, it[2](pv, v) end
    return v
end

local ExprFactory = {__name = 'ExprFactory'}

function ExprFactory:__index(k)
    return setmetatable({[ekey] = {r = locals(2), [0] = 0}}, Expr)[k]
end

function ExprFactory:__call(t, root)
    if not oo.isinstance(t, Test) then root = t end
    return setmetatable({[ekey] = {r = root, [0] = 0}}, Expr)
end

local Matcher = oo.class('Matcher')

function Matcher:__init(value, t, fail)
    self._v, self._t, self._f = value, t, fail
end

function Matcher:_value()
    local v = self._v
    local ok, e = pcall(function() return getmetatable(v).__eval end)
    return v, ok and e or nil
end

function Matcher:_eval()
    if not self._has_ev then
        local v, e = self:_value()
        if e then v = e(v) end
        self._ev, self._has_ev = v, true
    end
    return self._ev
end

function Matcher:_fail(format, ...) return self._f(self._t, format, ...) end

function Matcher:label(s, ...)
    self._l = s:format(...)
    return self
end

function Matcher:fix(fn)
    self._ev = fn(self:_eval())
    return self
end

function Matcher:eq(want, cmp) return self:_rel_op(want, cmp or util.eq, '') end
function Matcher:lt(want) return self:_rel_op(want, util.lt, '<') end
function Matcher:lte(want) return self:_rel_op(want, util.lte, '<=') end
function Matcher:gt(want) return self:_rel_op(want, util.gt, '>') end
function Matcher:gte(want) return self:_rel_op(want, util.gte, '>=') end

function Matcher:_rel_op(want, cmp, op)
    return self:_compare(want, cmp, function()
        return ('%s%s'):format(op, util.repr(want))
    end)
end

function Matcher:close_to(want, eps)
    return self:_compare(
        want, function(g, w) return w - eps <= g and g <= w + eps end,
        function() return ('%s ±%s'):format(util.repr(want), util.repr(eps)) end)
end

function Matcher:close_to_rel(want, fact)
    return self:close_to(want, fact * want)
end

function Matcher:_compare(want, cmp, fmt)
    local got = self:_eval()
    if not cmp(got, want) then
        self:_fail("%s = @{+WHITE}%s@{NORM}, want @{+CYAN}%s@{NORM}",
                   self._l or util.repr(self._v), util.repr(got), fmt)
    end
    return self
end

function Matcher:raises(want)
    local v, e = self:_value()
    local ok, err = pcall(function() if e then e(v) else v() end end)
    if ok then
        self:_fail("%s didn't raise an error", self._l or util.repr(v))
    elseif not err:find(want) then
        self:_fail("%s raised an error that doesn't match @{+CYAN}%s@{NORM}\n"
                   .. "  @{+WHITE}%s@{NORM}", self._l or util.repr(v),
                   util.repr(want), err)
    end
end

local PASS = '@{+GREEN}PASS@{NORM}'
local SKIP = '@{+YELLOW}SKIP@{NORM}'
local FAIL = '@{+RED}FAIL@{NORM}'
local ERROR = '@{+RED}ERROR@{NORM}'

Test = oo.class('Test')
Test.helper = 'xGJLirXXSePWnLIqptqM85UBIpZdaYs86P7zfF9sWaa3'
Test.expr = setmetatable({}, ExprFactory)

function Test:__init(name, parent)
    self.name, self._parent = name, parent
    if not parent then
        self.npass, self.nskip, self.nfail, self.nerror = 0, 0, 0, 0
    end
end

function Test:cleanup(fn) self._cleanups = list.append(self._cleanups, fn) end

function Test:patch(tab, name, value)
    local old = rawget(tab, name)
    self:cleanup(function() rawset(tab, name, old) end)
    rawset(tab, name, value)
end

function Test:repr(v)
    return function() return util.repr(v) end
end

function Test:func(name, args)
    return function()
        local parts = list()
        for _, arg in list.ipairs(args) do parts:append(util.repr(arg)) end
        return ('%s(%s)'):format(name, table.concat(parts, ', '))
    end
end

function Test:printf(format, ...) return io.aprintf(format, ...) end

function Test:log(format, ...) return self:_log('', format, ...) end

function Test:skip(format, ...)
    self._skip = true
    self:_log(SKIP, format, ...)
    error(err_terminate)
end

function Test:error(format, ...)
    self._fail = true
    self:enable_output()
    return self:_log(FAIL, format, ...)
end

function Test:fatal(format, ...)
    self:error(format, ...)
    error(err_terminate)
end

function Test:expect(cond, format, ...)
    return self:_expect(cond, self.error, format, ...)
end

function Test:assert(cond, format, ...)
    return self:_expect(cond, self.fatal, format, ...)
end

function Test:_expect(cond, fail, format, ...)
    if not format then return Matcher(cond, self, fail) end
    if not cond then return fail(self, format, ...) end
end

function Test:_log(prefix, format, ...)
    local args = list.pack(...)
    for i, arg in args:ipairs() do
        if type(arg) == 'function' then args[i] = arg() end
    end
    local msg = io.aformat(format, args:unpack())
    return io.printf('%s%s%s%s%s', io.ansi(prefix), prefix ~= '' and ': ' or '',
                     self:_location(1), msg, msg:sub(-1) ~= '\n' and '\n' or '')
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
                if not name then
                    return io.aformat('@{CYAN}%s:%s@{NORM}: ', src, line)
                end
                if value == self.helper then break end
                i = i + 1
            end
        end
        level = level + 1
    end
end

function Test:failed()
    if self._error or self._fail then return true end
    for _, child in list.ipairs(self.children) do
        if child:failed() then return true end
    end
    return false
end

function Test:result()
    return io.ansi(self._error and ERROR or self:failed() and FAIL
                   or self._skip and SKIP or PASS)
end

function Test:run(name, fn)
    local t = Test(name, self)
    if t:_run(fn) then self.children = list.append(self.children, t) end
    t = nil
    collectgarbage('collect')
end

function Test:_run(fn)
    self:_progress_tick()
    self:_capture_output()
    local start = time.get_absolute_time()
    self:_pcall(fn, self)
    self.duration = time.get_absolute_time() - start
    for i = list.len(self._cleanups), 1, -1 do
        self:_pcall(self._cleanups[i])
    end
    self._cleanups = nil
    self:_restore_output()
    local keep = self.children or self:failed()
    self:_up(function(t)
        keep = keep or t._full_results
        if not t.npass then return end
        if self._error then t.nerror = t.nerror + 1
        elseif self:failed() then t.nfail = t.nfail + 1
        elseif self._skip then t.nskip = t.nskip + 1
        else t.npass = t.npass + 1 end
    end)
    self._parent, self._out, self._old_out = nil, nil, nil
    return keep
end

function Test:_pcall(fn, ...)
    return xpcall(fn, function(err)
        if err ~= err_terminate then
            self._error = true
            self:enable_output()
            return self:_log(ERROR, "%s", debug.traceback(err, 2))
        end
        return err
    end, ...)
end

function Test:_progress_tick()
    local progress = self:_attr('_progress')
    if progress then return progress:write('.') end
end

function Test:_progress_end()
    self:_up(function(t)
        if not t._parent and t._progress then
            t._progress:write('\n')
            t._progress = nil
        end
    end)
end

function Test:enable_output()
    if not self._out then return end
    self:_progress_end()
    local out = self._out
    self._out = nil
    self:_restore_output()
    if self._parent then self._parent:enable_output() end
    self._old_out = _G.stdout
    local level = -1
    self:_up(function(t) level = level + 1 end)
    io.aprintf("@{BLUE}%s@{NORM} @{+WHITE}%s@{NORM} @{BLUE}%s@{NORM}\n",
               ('-'):rep(2 * level), self.name,
               ('-'):rep(77 - 2 * level - #self.name))
    out:replay(stdout)
end

function Test:_capture_output()
    self._out = Buffer(self)
    self._old_out, _G.stdout = _G.stdout, self._out
end

function Test:_restore_output()
    if not self._old_out then return end
    _G.stdout = self._old_out
end

function Test:_up(fn)
    local t = self
    while t do
        local res = fn(t)
        if res ~= nil then return res end
        t = t._parent
    end
end

function Test:_attr(name)
    return self:_up(function(t) return t[name] end)
end

local fn_comp = util.table_comp{1, 2}

function Test:run_module(name, pat)
    pat = pat or def_func_pat
    local module = require(name)
    self:cleanup(function() package.loaded[name] = nil end)
    local ok, fn = pcall(function() return module.set_up end)
    if ok and fn then fn(self) end
    local fns = list()
    for name, fn in pairs(module) do
        if name:find(pat) then
            local info = debug.getinfo(fn, 'S')
            fns:append({info and info.linedefined or 0, name, fn})
        end
    end
    for _, fn in util.sort(fns, fn_comp):ipairs() do
        local y = fn[2]:find(yield_pat)
        self:run(fn[2] .. (y and " (yield)" or ""), fn[3])
        if y then
            self:run(fn[2] .. " (no yield)", function(t)
                local save = yield_enabled(false)
                t:cleanup(function() yield_enabled(save) end)
                return fn[3](t)
            end)
        end
    end
end

function Test:run_modules(mod_pat, func_pat)
    mod_pat = mod_pat or def_mod_pat
    func_pat = func_pat or def_func_pat
    for _, name in util.sort(util.keys(package.preload)):ipairs() do
        if name:find(mod_pat) then
            self:run(name, function(t) t:run_module(name, func_pat) end)
        end
    end
end

function Test:main(runs)
    io.aprintf("@{CLR}Running tests\n")
    self._progress = stdout
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
    self:_progress_end()
    io.write("\n")
    self:print_result()
    if list.len(self.children) > 0 then io.write("\n") end
    io.printf("Tests: %d passed, %d skipped, %d failed, %d errored, %d total\n",
              self.npass, self.nskip, self.nfail, self.nerror,
              self.npass + self.nskip + self.nfail + self.nerror)
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
    for _, t in list.ipairs(self.children) do t:print_result(indent) end
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
        -- TODO: Show full results up to level x
        -- TODO: Terminate on first failure
        -- TODO: Launch repl on failure
        if line == 'fo' then full_output = not full_output
        elseif line == 'fr' then full_results = not full_results
        elseif line == 'q' then break
        elseif line:sub(1, 1) == 'r' then
            runs = math.tointeger(tonumber(line:sub(2))) or 1
        end
    end
end

function main()
    return xpcall(pmain, function(err)
        io.printf("ERROR: %s\n", debug.traceback(err, 2))
        return err
    end)
end
