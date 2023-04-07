_ENV = require 'module'(...)

local int64 = require 'int64'
local math = require 'math'
local string = require 'string'
local table = require 'table'
local testing = require 'testing'

local function arguments(args)
    local fmt = '('
    for i, arg in ipairs(args) do
        if i > 1 then fmt = fmt .. ', ' end
        if (type(arg) == 'number' and math.type(arg) ~= 'integer')
                or (type(arg) == 'userdata' and arg.__name == 'int64') then
            fmt = fmt .. '%s'
        else
            fmt = fmt .. '%q'
        end
    end
    fmt = fmt .. ')'
    return string.format(fmt, table.unpack(args))
end

function test_limits(t)
    t:expect(int64.max == int64('0x7fffffffffffffff'), "int64.max")
    t:expect(int64.min == int64('0x8000000000000000'), "int64.min")
    t:expect(int64.max + 1 == int64.min, "int64.max + 1")
    t:expect(int64.min - 1 == int64.max, "int64.min - 1")
end

function test_hex(t)
    for _, test in ipairs{
        {{int64(0)}, '0'},
        {{int64(0x1234)}, '1234'},
        {{int64(-0x1234)}, 'ffffffffffffedcc'},
        {{int64('12345678901234567890')}, 'ab54a98ceb1f0ad2'},
        {{int64(0x1234), 1}, '1234'},
        {{int64(0x1234), 8}, '00001234'},
        {{int64(0x1234), 16}, '0000000000001234'},
    } do
        local args, want = table.unpack(test)
        t:run(arguments(args), function(t)
            local got = int64.hex(table.unpack(args))
            t:expect(got == want, "got %q, want %q", got, want)
        end)
    end
end

function test_tointeger(t)
    -- TODO
end

function test_cast_to_int64(t)
    for _, test in ipairs{
        -- Conversion from boolean
        {{false}, int64(0)},
        {{true}, int64(1)},
        -- Conversion from integer
        {{0x12345678}, int64('0x12345678')},
        {{0x12345678, 0x9abcdef0}, int64('0x9abcdef012345678')},
        {{-1, 0x123}, int64('0x123ffffffff')},
        -- Conversion from float
        {{12345678.0}, int64('12345678')},
        {{-12345678.0}, int64('-12345678')},
        {{1.2}, nil},                               -- Not an integer
        {{1e19}, nil},                              -- Doesn't fit
        {{-1e19}, nil},
        -- Conversion from string
        {{'0'}, int64(0)},                          -- Decimal
        {{'1234567890'}, int64(1234567890)},
        {{'+1234567890'}, int64(1234567890)},
        {{'-1234567890'}, int64(-1234567890)},
        {{'1234567890', 10}, int64(1234567890)},
        {{'0x12abcdef'}, int64(0x12abcdef)},        -- Hexadecimal
        {{'+0X12aBcDeF'}, int64(0x12abcdef)},
        {{'-0x12abcdef'}, int64(-0x12abcdef)},
        {{'12abcdef', 16}, int64(0x12abcdef)},
        {{'0xabcdefg'}, nil},
        {{'0o1234567'}, int64(0x53977)},            -- Octal
        {{'-0o1234567'}, int64(-0x53977)},
        {{'1234567', 8}, int64(0x53977)},
        {{'0o12345678'}, nil},
        {{'0b0001001000110100'}, int64(0x1234)},    -- Binary
        {{'-0b0001001000110100'}, int64(-0x1234)},
        {{'0001001000110100', 2}, int64(0x1234)},
        {{'0b12'}, nil},
        {{'xYz', 36}, int64((((33 * 36) + 34) * 36) + 35)},     -- Base 36
        {{'  1234567890   '}, int64(1234567890)},   -- Whitespace
        {{''}, nil},                                -- No digits
        {{'   '}, nil},
        {{'1234567890x'}, nil},                     -- Non-digit suffix
        {{'0xabcdef0123456789'}, int64(0x23456789, 0xabcdef01)},    -- Negative
    } do
        local args, want = table.unpack(test)
        t:run(arguments(args), function(t)
            local got = int64(table.unpack(args))
            t:expect(got == want, "got %s, want %s", got, want)
        end)
    end
end

function test_unary_ops(t)
    local values = {
        0, -1, 1, -7, 13, -12345678, 12345678,
        math.mininteger + 1, math.maxinteger,
    }
    local ops = {
        ['-'] = function(v) return -v end,
        ['~'] = function(v) return ~v end,
    }
    for _, op in ipairs(testing.sorted_keys(ops)) do
        local f = ops[op]
        for _, value in ipairs(values) do
            t:run(string.format("%s(%d)", op, value), function(t)
                local got, want = f(int64(value)), int64(f(value))
                t:expect(got == want, "got %s, want %s", got, want)
            end)
        end
    end

    -- TODO: Test a few numbers that don't fit an integer
end

function test_binary_int_ops(t)
    local values = {-1, 3, -7, 13, -1234, 2468}
    local ops = {
        ['+'] = function(a, b) return a + b end,
        ['-'] = function(a, b) return a - b end,
        ['*'] = function(a, b) return a * b end,
        ['//'] = function(a, b) return a // b end,
        ['%'] = function(a, b) return a % b end,
        ['&'] = function(a, b) return a & b end,
        ['|'] = function(a, b) return a | b end,
        ['~'] = function(a, b) return a ~ b end,
    }
    for _, op in ipairs(testing.sorted_keys(ops)) do
        local f = ops[op]
        for _, a in ipairs(values) do
            for _, b in ipairs(values) do
                t:run(string.format("%d %s %d", a, op, b), function(t)
                    local got, want = f(int64(a), int64(b)), int64(f(a, b))
                    t:expect(got == want, "got %s, want %s", got, want)
                end)
            end
        end
    end

    -- TODO: Test a few numbers that don't fit an integer
end

function test_binary_float_ops(t)
    local values = {0, -1, 3, -7, 13, -1234, 2468}
    local ops = {
        ['/'] = function(a, b) return a / b end,
        ['^'] = function(a, b) return a ^ b end,
    }
    for _, op in ipairs(testing.sorted_keys(ops)) do
        local f = ops[op]
        for _, a in ipairs(values) do
            for _, b in ipairs(values) do
                t:run(string.format("%d %s %d", a, op, b), function(t)
                    local got, want = f(int64(a), int64(b)), f(a, b)
                    t:expect(got == want, "got %s, want %s", got, want)
                end)
            end
        end
    end
end

function test_shift_ops(t)
    -- TODO
end

function test_relational_ops(t)
    -- TODO
end

function test_tostring(t)
    -- TODO
end

function test_concat(t)
    if not pcall(function() return 1 .. 2 end) then
        t:skip("automatic number => string conversions disabled")
    end
    -- TODO
end
