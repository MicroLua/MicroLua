_ENV = require 'module'(...)

local io = require 'io'
local oo = require 'oo'
local package = require 'package'
local string = require 'string'
local table = require 'table'


local def_mod_pat = '%.tests$'
local def_func_pat = '^test_'
local err_end = {}

function sorted_keys(tab, filter)
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
    self.skipped = false
    self.children = {}
end

function Test:message(format, ...)
    if format:sub(-1) ~= '\n' then format = format .. '\n' end
    io.write(string.format(format, ...))
    -- TODO: Include location
end

function Test:error(format, ...)
    self.errors = self.errors + 1
    self:message("ERROR: " .. format, ...)
end

function Test:fatal(format, ...)
    self:error(format, ...)
    error(err_end)
end

function Test:skip(format, ...)
    self.skipped = true
    self:message("SKIP: " .. format, ...)
    error(err_end)
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
    -- local b = Buffer()
    -- TODO: io.output argumnet must be an io.file
    -- TODO: Define own io abstraction, don't use io at all, or wrap it
    -- TODO: Override lua_write*() macros to make print() work, or maybe disable
    --       them altogether and override print()
    io.write(string.format("----- BEGIN %s\n", name))
    local res, err = pcall(func, t)
    io.write(string.format("----- END   %s\n", name))
    if not res then
        if err ~= err_end then t:error("%s", err) end
    end
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
    io.write(string.format("%s%s: %s\n", indent, self:result(),
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
    io.write(string.format("\nResult: %s\n\n", t:result()))
end

Buffer = oo.class('Buffer')

function Buffer:__init()
    self.data = {}
end

function Buffer:write(...)
    for _, d in table.pack(...) do
        self.data[#self.data + 1] = d
    end
end

function Buffer:value()
    return table.concat(self.data)
end
