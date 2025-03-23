-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local debug = require 'debug'
local math = require 'math'
local cli = require 'mlua.cli'
local io = require 'mlua.io'
local list = require 'mlua.list'
local oo = require 'mlua.oo'
local repr = require 'mlua.repr'
local thread = try(require, 'mlua.thread')
local time = require 'mlua.time'
local util = require 'mlua.util'
local package = require 'package'
local string = require 'string'

local def_mod_pat = '^(.*)%.test$'
local def_func_pat = '^test_'
local blocking_pat = '_BNB$'
local err_terminate = {}

local module_name = ...

local Recorder = oo.class('Recorder', io.Recorder)

function Recorder:__init(t) self.t = t end

function Recorder:write(...)
    io.Recorder.write(self, ...)
    if not self:is_empty() and self.t._root._opts.output then
        self.t:enable_output()
    end
end

local function format_call(name, args, first)
    local parts = list()
    for i = first or 1, list.len(args) do parts:append(repr(args[i])) end
    return ('%s(%s)'):format(name, parts:concat(', '))
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
            local i, env = 1
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
            return ('%s[%s]'):format(s, repr(k))
        end,
        function(pv, v) return v[k] end,
    })
    return self
end

function Expr:__call(...)
    local args = list.pack(...)
    list.append(rawget(self, ekey), {
        function(s)
            local first = 1
            if rawequal(args[1], self) then
                s = s:gsub('%.([%a_][%w_]*)$', ':%1')
                first = 2
            end
            return format_call(s, args, first)
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
    local pv, v, le = nil, e.r, list.len(e)
    for i = 1, le - 1 do pv, v = v, e[i][2](pv, v) end
    if e.m then return list.pack(e[le][2](pv, v)) end
    return e[le][2](pv, v)
end

local ExprFactory = {__name = 'ExprFactory'}

local function new_expr(fact, attrs)
    local e = rawget(fact, ekey)
    if e then for k, v in pairs(e) do attrs[k] = v end end
    return setmetatable({[ekey] = attrs}, Expr)
end

function ExprFactory:__index(k) return new_expr(self, {r = locals(2)})[k] end
function ExprFactory:__call(root) return new_expr(self, {r = root}) end

local Matcher = oo.class('Matcher')

function Matcher:__init(value, t, fail)
    self._v, self._t, self._f = value, t, fail
end

function Matcher:_value()
    local v = self._v
    local ok, e = pcall(function() return rawget(getmetatable(v), '__eval') end)
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

function Matcher:label(format, ...)
    self._l = format:format(...)
    return self
end

function Matcher:func(name, ...)
    self._l = format_call(name, list.pack(...))
    return self
end

function Matcher:op(op)
    self._op = op
    return self
end

function Matcher:apply(fn)
    self._ev = fn(self:_eval())
    return self
end

function Matcher:fmt(fn)
    self._fmt = fn
    return self
end

function Matcher:_label() return self._l or repr(self._v) end
function Matcher:_repr(v) return (self._fmt or repr)(v) end

function Matcher:eq(want, eq) return self:_rel_op(want, eq or equal, '') end

function Matcher:neq(want, eq)
    return self:_rel_op(
        want, function(a, b) return not (eq or equal)(a, b) end, '~=')
end

function Matcher:lt(want) return self:_rel_op(want, util.lt, '<') end
function Matcher:lte(want) return self:_rel_op(want, util.lte, '<=') end
function Matcher:gt(want) return self:_rel_op(want, util.gt, '>') end
function Matcher:gte(want) return self:_rel_op(want, util.gte, '>=') end

function Matcher:_rel_op(want, cmp, op)
    return self:_compare(want, cmp, function()
        return ('%s%s'):format(op, self:_repr(want))
    end)
end

function Matcher:eq_one_of(want, eq)
    eq = eq or equal
    return self:_rel_op(want,
        function(g, w)
            for _, v in list.ipairs(w) do
                if eq(g, v) then return true end
            end
            return false
        end,
        "one of ")
end

function Matcher:close_to(want, eps)
    return self:_compare(
        want, function(g, w) return w - eps <= g and g <= w + eps end,
        function() return ('%s ±%s'):format(self:_repr(want), repr(eps)) end)
end

function Matcher:close_to_rel(want, fact)
    return self:close_to(want, fact * want)
end

function Matcher:_compare(want, cmp, fmt)
    local got = self:_eval()
    if not cmp(got, want) then
        self:_fail("%s %s @{+WHITE}%s@{NORM}, want @{+CYAN}%s@{NORM}",
                   self:_label(), self._op or '=', self:_repr(got), fmt)
    end
    return self
end

function Matcher:matches(want)
    local got = self:_eval()
    if type(got) ~= 'string' then
        self:_fail("%s = @{+WHITE}%s@{NORM}, isn't a string",
                   self:_label(), self:_repr(got))
    elseif not got:find(want) then
        self:_fail("%s = @{+WHITE}%s@{NORM}, doesn't match @{+CYAN}%s@{NORM}",
                   self:_label(), self:_repr(got), self:_repr(want))
    end
    return self
end

function Matcher:has(key) return self:_index(key, true, "doesn't have") end
function Matcher:not_has(key) return self:_index(key, false, "has") end

function Matcher:_index(key, want, text)
    local v = self:_eval()
    local ok, value = pcall(function() return v[key] end)
    if (ok and value ~= nil) ~= want then
        self:_fail("%s %s key @{+CYAN}%s@{NORM}", self:_label(), text,
                   repr(key))
    end
    return self
end

function Matcher:raises(want)
    local v, e = self:_value()
    local ok, err = pcall(function() if e then e(v) else v() end end)
    if ok then
        self:_fail("%s didn't raise an error", self:_label())
    elseif want and type(err) == 'string' then
        if not err:find(want) then
            self:_fail("%s raised @{+WHITE}%s@{NORM}, doesn't match @{+CYAN}%s@{NORM}",
                       self:_label(), self:_repr(err), repr(want))
        end
    elseif want and err ~= want then
        self:_fail("%s raised @{+WHITE}%s@{NORM}, want @{+CYAN}%s@{NORM}",
                   self:_label(), self:_repr(err), self:_repr(want))
    end
end

local PASS = '@{+GREEN}PASS@{NORM}'
local SKIP = '@{+YELLOW}SKIP@{NORM}'
local FAIL = '@{+RED}FAIL@{NORM}'
local ERROR = '@{+RED}ERROR@{NORM}'

Test = oo.class('Test')
Test.helper = 'xGJLirXXSePWnLIqptqM85UBIpZdaYs86P7zfF9sWaa3'
Test.expr = setmetatable({}, ExprFactory)
Test.mexpr = setmetatable({[ekey] = {m = true}}, ExprFactory)

function Test:__init(name, parent)
    self.name, self._parent = name, parent
    self._root, self._level = self:_up(function(t)
        if not t._parent then return t end
    end)
    if not parent then
        self.npass, self.nskip, self.nfail, self.nerror = 0, 0, 0, 0
    end
end

function Test:path()
    local path = self.name
    self:_up(function(t)
        if t == self or not t._parent then return end
        path = ('%s/%s'):format(t.name, path)
    end)
    return path
end

function Test:cleanup(fn) self._cleanups = list.append(self._cleanups, fn) end

function Test:patch(tab, name, value)
    local old = rawget(tab, name)
    self:cleanup(function() rawset(tab, name, old) end)
    rawset(tab, name, value)
    return value
end

local onces = {}

function Test:once(id, fn)
    if onces[id] then return end
    fn()
    onces[id] = true
end

function Test:repr(v)
    return function() return repr(v) end
end

function Test:func(name, args)
    return function()
        local parts = list()
        for _, arg in list.ipairs(args) do parts:append(repr(arg)) end
        return ('%s(%s)'):format(name, parts:concat(', '))
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
    if not self._fail then self:_up(function(t) t._fail = true end) end
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

function Test:context(ctx) self._ctx = ctx end

function Test:_log(prefix, format, ...)
    local args = list.pack(...)
    for i, arg in args:ipairs() do
        if type(arg) == 'function' then args[i] = arg() end
    end
    local msg = io.aformat(format, args:unpack())
    return io.printf('%s%s%s%s%s%s',
                     io.ansi(prefix), prefix ~= '' and ': ' or '',
                     self:_location(1), msg, msg:sub(-1) ~= '\n' and '\n' or '',
                     self:_context())
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

function Test:_context()
    local ctx = self._ctx
    if not ctx then return '' end
    local parts = list()
    for _, k in util.keys(ctx):sort():ipairs() do
        local v = ctx[k]
        if type(v) == 'function' then v = v() end
        if type(v) ~= 'string' then v = repr(v) end
        parts:append(io.aformat('  @{+MAGENTA}%s@{NORM}: %s\n', k, v))
    end
    return parts:concat()
end

function Test:failed() return self._error or self._fail end

function Test:_result()
    return self._error and ERROR or self._fail and FAIL
           or self._skip and SKIP or PASS
end

function Test:run(name, fn) return Test(name, self):_run(fn) end

function Test:_pre_run()
    collectgarbage()
    local count, size, used = alloc_stats(true)
    if not count then return end
    self._alloc_count, self._alloc_size = count, size
    self._alloc_base, self._alloc_peak = used, used
    if thread then
        self._th_disps, self._th_waits, self._th_resumes = thread.stats()
    end
end

function Test:_post_run()
    if self._th_disps then
        local disps, waits, resumes = thread.stats()
        self._th_disps = disps - self._th_disps
        self._th_waits = waits - self._th_waits
        self._th_resumes = resumes - self._th_resumes
    end
    collectgarbage()
    local count, size, used, peak = alloc_stats()
    if not count then return end
    self._alloc_count = count - self._alloc_count
    self._alloc_size = size - self._alloc_size
    self._alloc_used = used
    return self:_up(function(t)
        if peak > t._alloc_peak then t._alloc_peak = peak end
    end)
end

function Test:_print_stats(out)
    if self._alloc_count then
        io.fprintf(
            out, "Allocs: %s (%s B), peak: %s B (+%s B), used: %s B\n",
            self._alloc_count, self._alloc_size, self._alloc_peak,
            self._alloc_peak - self._alloc_base, self._alloc_used)
    end
    if self._th_disps and self._th_disps ~= 0 then
        io.fprintf(out, "Thread: %s dispatches, %s waits, %s resumes\n",
                   self._th_disps, self._th_waits, self._th_resumes)
    end
end

function Test:_run(fn)
    self:_capture_output()
    self:_pre_run()
    local start = time.ticks()
    self:_pcall(fn, self)
    local duration = time.ticks() - start
    self._ctx = nil
    for i = list.len(self._cleanups), 1, -1 do
        self:_pcall(self._cleanups[i])
    end
    self._cleanups, self._ctx = nil

    -- Compute stats.
    self:_post_run()
    self:_restore_output()
    local root = self._root
    if self._error then root.nerror = root.nerror + 1
    elseif self._fail then root.nfail = root.nfail + 1
    elseif self._skip then root.nskip = root.nskip + 1
    else root.npass = root.npass + 1 end

    -- Output results and stats.
    local level, opts = self._level, root._opts
    if level >= 0 and level < opts.results then
        local out = root._stdout
        local indent = (' '):rep(2 * level)
        local left = ('%s%s: %s'):format(indent, self:_result(), self.name)
        local right = ('%.3f s'):format(duration / time.sec)
        io.fprintf(out, "%s%s %s\n", io.ansi(left),
                   (' '):rep(78 - #io.ansi(left, io.empty_tags) - #right),
                   right)
        if opts.stats then
            self:_print_stats(io.Indenter(out, indent .. ' '))
        end
    end
end

local tb_exclude = {
    "\n\tmlua%.testing:[^\n]*",
    "\n\t%[C%]: in function 'xpcall'[^\n]*",
    "\n\t%(%.%.%.tail calls%.%.%.%)[^\n]*"
}

function Test:_pcall(fn, ...)
    return xpcall(fn, function(err)
        if err ~= err_terminate then
            self._error = true
            self:_up(function(t) if t ~= self then t._fail = true end end)
            self:enable_output()
            local tb = debug.traceback(err, 2)
            if type(tb) == 'string' then
                for _, pat in ipairs(tb_exclude) do tb = tb:gsub(pat, '') end
            end
            return self:_log(ERROR, "%s", tb)
        end
        return err
    end, ...)
end

function Test:_progress_show()
    local root, path = self._root, self:path()
    if not path then return end
    return io.afprintf(root._stdout,
                       '@{SAVE}Running: @{CYAN}%s@{NORM}@{RESTORE}', path)
end

function Test:_progress_clear()
    local root = self._root
    return io.afprintf(root._stdout, '@{CLREOS}')
end

function Test:enable_output()
    if not self._out then return end
    local out = self._out
    self._out = nil
    self:_restore_output()
    if self._parent then self._parent:enable_output() end
    self._old_out = _G.stdout
    if not self._parent then return end
    local _, level = self:_up(function(t) end)
    io.aprintf("@{BLUE}%s@{NORM} @{+WHITE}%s@{NORM} @{BLUE}%s@{NORM}\n",
               ('-'):rep(2 * level), self.name,
               ('-'):rep(77 - 2 * level - #self.name))
    out:replay(stdout)
end

function Test:_capture_output()
    self:_progress_show()
    self._out = Recorder(self)
    self._old_out, _G.stdout = _G.stdout, self._out
end

function Test:_restore_output()
    if not self._old_out then return end
    self:_progress_clear()
    _G.stdout, self._old_out = self._old_out
end

function Test:_up(fn)
    local t, level = self, -1
    while t do
        local res = fn(t)
        if res ~= nil then return res, level end
        t, level = t._parent, level + 1
    end
    return nil, level
end

local fn_comp = util.table_comp{1, 2}

function Test:run_module(name, pat)
    pat = pat or def_func_pat
    local root = self._root
    if not root._loaded then
        local loaded = {}
        for n in pairs(package.loaded) do loaded[n] = true end
        root._loaded = loaded
    end
    self:cleanup(function(t)
        local loaded = root._loaded
        for n in pairs(package.loaded) do
            if not loaded[n] then package.loaded[n] = nil end
        end
    end)
    local module = require(name)
    local ok, fn = pcall(function() return module.set_up end)
    if ok and fn then fn(self) end
    local fns = list()
    for name, fn in pairs(module) do
        if name:find(pat) then
            local info = debug.getinfo(fn, 'S')
            fns:append({info and info.linedefined or 0, name, fn})
        end
    end
    for _, fn in fns:sort(fn_comp):ipairs() do
        local b = thread and fn[2]:find(blocking_pat)
        self:run(fn[2] .. (b and " (non-blocking)" or ""), fn[3])
        if b then
            self:run(fn[2] .. " (blocking)", function(t)
                local save = thread.blocking(true)
                t:cleanup(function() thread.blocking(save) end)
                return fn[3](t)
            end)
        end
    end
end

function Test:run_modules(mod_pat, func_pat)
    mod_pat = mod_pat or def_mod_pat
    func_pat = func_pat or def_func_pat
    local mods = list()
    for name in pairs(package.preload) do
        if name:find(mod_pat) then mods:append(name) end
    end
    mods:sort(function(a, b) return a:match(mod_pat) < b:match(mod_pat) end)
    for _, name in mods:ipairs() do
        self:run(name, function(t) t:run_module(name, func_pat) end)
    end
end

function Test:_main(runs)
    io.aprintf("@{CLR}@{HIDE}")
    local done<close> = function() return io.aprintf("@{SHOW}") end
    self._stdout = stdout
    local start = time.ticks()
    self:_run(function(t)
        if runs == 1 then return t:run_modules() end
        for i = 1, runs do
            t:run(("Run #%s"):format(i), function(t) return t:run_modules() end)
        end
    end)
    local dt = time.ticks() - start
    io.printf("\nTests: %d passed, %d skipped, %d failed, %d errored, %d total in %.3f s\n",
              self.npass, self.nskip, self.nfail, self.nerror,
              self.npass + self.nskip + self.nfail + self.nerror, dt / time.sec)
    self:_print_stats(stdout, '')
    io.printf("Result: %s\n", io.ansi(self:_result()))
end

local reg_exclude = {
    [1] = true, [2] = true,
    _CLIBS = true, _LOADED = true, _PRELOAD = true, ['_UBOX*'] = true,
}

local function print_registry()
    local reg = debug.getregistry()
    for k, v in pairs(reg) do
        if not reg_exclude[k] then
            if util.get(v, '__name') ~= k then v = repr(v) end
            io.aprintf("[@{+WHITE}%s@{NORM}] = %s\n", repr(k), v)
        end
    end
end

local Runner = oo.class('Runner')

function Runner:__init(opts) self.opts = opts end

function Runner:run()
    local failed
    while not self.exit do
        local t = Test()
        t._opts = self.opts
        t:_main(self.opts.runs)
        failed = t:failed()
        if not self.opts.prompt then break end
        self:prompt()
    end
    return not failed
end

function Runner:prompt()
    while true do
        io.printf("\n(testing) ")
        local line = ''
        while true do
            local c = stdin:read(1)
            if c == '\n' then break end
            line = line .. c
        end
        local args, cmd = list()
        for t in line:gmatch('[^%s]+') do
            if not cmd then cmd = t else args:append(t) end
        end
        if not cmd then break end
        local fn = self['cmd_' .. cmd]
        if fn then
            if fn(self, args:unpack()) then break end
        else
            io.aprintf("@{+RED}ERROR:@{NORM} unknown command: %s\n", cmd)
        end
    end
end

-- TODO: Terminate on first failure
-- TODO: Launch repl on failure

function Runner:cmd_out()
    self.opts.output = not self.opts.output
end

function Runner:cmd_r(count)
    self.opts.runs = math.tointeger(tonumber(count)) or 1
    return true
end

function Runner:cmd_reg() print_registry() end

function Runner:cmd_res(level)
    local opts = self.opts
    if level then
        opts.results = math.tointeger(tonumber(level)) or math.maxinteger
    else
        opts.results = opts.results == 0 and math.maxinteger or 0
    end
end

function Runner:cmd_st(level)
    local opts = self.opts
    if level then
        opts.stats = true
        self:cmd_res(level)
    elseif not opts.stats then
        opts.stats = true
        if opts.results <= 0 then opts.results = math.maxinteger end
    else
        opts.stats = false
    end
end

function Runner:cmd_x()
    self.exit = true
    return true
end

local function pmain(opts, args)
    local argv = util.get(_G, 'arg')
    local opts, args = cli.parse_args(argv)
    cli.parse_opts(opts, {
        output = cli.bool_opt(false),
        prompt = cli.bool_opt(true),
        results = cli.int_opt(0),
        runs = cli.int_opt(1),
        stats = cli.bool_opt(false),
    })
    return Runner(opts):run()
end

function main()
    local ok, res = xpcall(pmain, function(err)
        io.aprintf("\n@{+RED}ERROR:@{NORM} %s\n", debug.traceback(err, 2))
        return err
    end)
    return ok and res
end

overrides = {}
try(require, 'mlua.testing.platform')
