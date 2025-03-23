-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local math = require 'math'
local int64 = require 'mlua.int64'
local util = require 'mlua.util'
local string = require 'string'
local table = require 'table'

local integer_bits = string.packsize('j') * 8
local number_bits = string.packsize('n') * 8

function set_up(t)
    local v = int64(0)
    t:printf("integer: %d bits, number: %d bits, int64 type: %s\n",
             integer_bits, number_bits, math.type(v) or type(v))
end

function test_limits(t)
    for _, test in ipairs{
        {"int64.min", int64.min, int64('0x8000000000000000')},
        {"int64.max", int64.max, int64('0x7fffffffffffffff')},
        {"int64.min - 1", int64.min - 1, int64.max},
        {"int64.max + 1", int64.max + 1, int64.min},
    } do
        local desc, value, want = table.unpack(test)
        t:expect(value):label(desc):eq(want)
    end
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
        t:expect(t.expr(int64).hex(table.unpack(args))):eq(want)
    end
end

function test_tointeger(t)
    for _, test in ipairs{
        {int64(0), 0},
        {int64(12345678), 12345678},
        {int64(-12345678), -12345678},
        {int64(math.maxinteger), math.maxinteger},
        {int64(math.mininteger), math.mininteger},
        {int64.max,
         math.maxinteger >= int64.max and 9223372036854775807 or nil},
        {int64.min,
         math.mininteger <= int64.min and -9223372036854775808 or nil},
    } do
        local value, want = table.unpack(test)
        t:expect(t.expr(int64).tointeger(value)):eq(want)
    end
end

function test_tonumber(t)
    for _, test in ipairs{
        {int64(0), 0.0},
        {int64(123456), 123456.0},
        {int64(-123456), -123456.0},
        {int64.max, 9223372036854775807.0},
        {int64.min, -9223372036854775808.0},
    } do
        local value, want = table.unpack(test)
        t:expect(t.expr(int64).tonumber(value)):eq(want)
    end
end

function test_cast_to_int64(t)
    local skip = {}
    for _, test in ipairs{
        -- No-op conversion from int64
        {{int64(1234)}, int64(1234)},
        -- Conversion from boolean
        {{false}, int64(0)},
        {{true}, int64(1)},
        -- Conversion from integer
        {{0x12345678}, int64('0x12345678')},
        {{0xabcdef01},                              -- Sign extension
         integer_bits >= 64 and int64('0xabcdef01')
                            or int64('0xffffffffabcdef01')},
        integer_bits >= 64 and skip or
            {{0x12345678, 0x9abcdef0}, int64('0x9abcdef012345678')},
        integer_bits >= 64 and skip or
            {{0xabcdef01, 0}, int64('0xabcdef01')},
        integer_bits >= 64 and skip or {{-1, 0x123}, int64('0x123ffffffff')},
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
        {{'0xabcdef0123456789'},                    -- Negative
            integer_bits < 64 and int64(0x23456789, 0xabcdef01)
            or int64(0xabcdef0123456789)},
    } do
        if test == skip then goto continue end
        local args, want = table.unpack(test)
        t:expect(t.expr.int64(table.unpack(args))):eq(want)
        ::continue::
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
    for op, f in pairs(ops) do
        for _, v in ipairs(values) do
            local got, want = f(int64(v)), f(v)
            t:expect(equal(got, want), "%s(%s) = %s, want %s", op, v, got, want)
        end
    end

    for _, test in ipairs{
        {'-', int64('12345678901234'), int64('-12345678901234')},
        {'-', int64('-12345678901234'), int64('12345678901234')},
        {'-', int64.max, int64('0x8000000000000001')},
        {'-', int64.min, int64.min},
        {'~', int64('0x0123456789abcdef'), int64('0xfedcba9876543210')},
        {'~', int64.max, int64.min},
        {'~', int64.min, int64.max},
    } do
        local op, v, want = table.unpack(test)
        local got = ops[op](v)
        t:expect(got == want, "%s(%s) = %s, want %s", op, v, got, want)
    end
end

local variants = {
    ['int64(%s) %s int64(%s)'] = function(f)
        return function(a, b) return f(int64(a), int64(b)) end
    end,
    ['%s %s int64(%s)'] = function(f)
        return function(a, b) return f(a, int64(b)) end
    end,
    ['int64(%s) %s %s'] = function(f)
        return function(a, b) return f(int64(a), b) end
    end,
}

local function run_binary_ops_tests(t, ops, as, bs)
    for op, f in pairs(ops) do
        for fmt, v in pairs(variants) do
            local fmt = fmt .. " = %s, want %s"
            local vf = v(f)
            for _, a in ipairs(as) do
                for _, b in ipairs(bs) do
                    local got, want = vf(a, b), f(a, b)
                    t:expect(equal(got, want), fmt, a, op, b, got, want)
                end
            end
        end
    end
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
    run_binary_ops_tests(t, ops, values, values)

    for _, test in ipairs{
        {int64('12345678901234'), '+', int64('24680135791113'),
         int64('37025814692347')},
        {int64('12345678901234'), '-', int64('24680135791113'),
         int64('-12334456889879')},
        {int64('12345678901234'), '*', int64('1357'),
         int64('16753086268974538')},
        {int64('12345678901234'), '//', int64('246801357911'),
         int64('50')},
        {int64('12345678901234'), '%', int64('246801357911'),
         int64('5611005684')},
        {int64('0x0123456789abcdef'), '&', int64('0xabcdef0123456789'),
         int64('0x0101450101014589')},
        {int64('0x0123456789abcdef'), '|', int64('0xabcdef0123456789'),
         int64('0xabefef67abefefef')},
        {int64('0x0123456789abcdef'), '~', int64('0xabcdef0123456789'),
         int64('0xaaeeaa66aaeeaa66')},
    } do
        local a, op, b, want = table.unpack(test)
        local got = ops[op](a, b)
        t:expect(got == want, "%s %s %s = %s, want %s", a, op, b, got, want)
    end
end

function test_binary_float_ops(t)
    local values = {-1, 3, -7, 13, -1234, 2468}
    local ops = {
        ['/'] = function(a, b) return a / b end,
        ['^'] = function(a, b) return a ^ b end,
    }
    run_binary_ops_tests(t, ops, values, values)

    for _, test in ipairs{
        {int64('12345678901234'), '/', int64('246801357911'),
         int64.tonumber(int64('12345678901234'))
         / int64.tonumber(int64('246801357911'))},
        {int64('12345678901234'), '^', int64('5'),
         int64.tonumber(int64('12345678901234')) ^ 5.0},
    } do
        local a, op, b, want = table.unpack(test)
        local got = ops[op](a, b)
        t:expect(got == want, "%s %s %s = %s, want %s", a, op, b, got, want)
    end
end

function test_shift_ops(t)
    local values = {0, 3, 7, 13, 1234, 2468}
    local shifts = {0, 1, -1, -7, 7, -13, 13, -19, 19}
    local ops = {
        ['<<'] = function(a, b) return a << b end,
        ['>>'] = function(a, b) return a >> b end,
    }
    run_binary_ops_tests(t, ops, values, shifts)

    ops['(ashr)'] = int64.ashr
    for _, test in ipairs{
        {int64('0x0123456789abcdef'), '<<', int64(0),
         int64('0x0123456789abcdef')},
        {int64('0x0123456789abcdef'), '<<', int64(60),
         int64('0xf000000000000000')},
        {int64('0xffffffffffffffff'), '<<', int64(64), int64(0)},
        {int64('0xfedcba9876543210'), '<<', int64(-60), int64('0xf')},
        {int64('0xffffffffffffffff'), '<<', int64(-64), int64(0)},
        {int64('0xfedcba9876543210'), '>>', int64(0),
         int64('0xfedcba9876543210')},
        {int64('0xfedcba9876543210'), '>>', int64(60), int64('0xf')},
        {int64('0xffffffffffffffff'), '>>', int64(64), int64(0)},
        {int64('0x0123456789abcdef'), '>>', int64(-60),
         int64('0xf000000000000000')},
        {int64('0xffffffffffffffff'), '>>', int64(-64), int64(0)},
        {int64('0x1234567890abcdef'), '(ashr)', int64(0),
         int64('0x1234567890abcdef')},
        {int64('0x1234567890abcdef'), '(ashr)', int64(60), int64('0x1')},
        {int64('0xabcdef1234567890'), '(ashr)', int64(60),
         int64('0xfffffffffffffffa')},
        {int64('0x1234567890abcdef'), '(ashr)', int64(-60),
         int64('0xf000000000000000')},
        {int64('0x1234567890abcdef'), '(ashr)', int64(-64), int64(0)},
        {int64.max, '(ashr)', int64(63), int64(0)},
        {int64.max, '(ashr)', int64(64), int64(0)},
        {int64.min, '(ashr)', int64(63), int64(-1)},
        {int64.min, '(ashr)', int64(64), int64(-1)},
    } do
        local a, op, b, want = table.unpack(test)
        local got = ops[op](a, b)
        t:expect(got == want, "%s %s %s = %s, want %s", int64.hex(a), op, b,
                 int64.hex(got), int64.hex(want))
    end
end

function test_relational_ops(t)
    local values = {0, -1, 3, -7, 13, -1234, 2468}
    local ops = {
        ['=='] = equal,
        ['<'] = function(a, b) return a < b end,
        ['<='] = function(a, b) return a <= b end,
    }
    run_binary_ops_tests(t, ops, values, values)

    local function check(t, f, a, op, b, want)
        local got = f(a, b)
        t:expect(got == want, "%s %s %s = %s, want %s", a, op, b, got, want)
    end

    local large_num = int64.tonumber(int64(1) << 60)
    local large_int64 = int64(large_num)
    for _, test in ipairs{
        -- Large int64
        {int64.max - 1, int64.max - 2, false, false},
        {int64.max - 1, int64.max - 1, true, false},
        {int64.max - 1, int64.max, false, true},
        -- int64 fits number
        {int64(3), 2.5, false, false},
        {int64(3), 3.0, true, false},
        {int64(3), 3.5, false, true},
        -- Number fits int64
        {large_int64, 0.999 * large_num, false, false},
        {large_int64, large_num, true, false},
        {large_int64, 1.001 * large_num, false, true},
        -- Number doesn't fit int64
        {int64(0), 3.5e25, false, true},
    } do
        local a, b, eq, lt = table.unpack(test)
        for _, cmps in ipairs{
            {'==', eq, eq},
            {'<', lt, not (lt or eq)},
            {'<=', lt or eq, not lt},
        } do
            local op, want, rwant = table.unpack(cmps)
            local f = ops[op]
            check(t, f, a, op, b, want)
            check(t, f, b, op, a, rwant)
            check(t, f, -a, op, -b, rwant)
            check(t, f, -b, op, -a, want)
        end
    end

    for _, test in ipairs{
        {int64(0), int64(0), true, false},
        {int64(0), int64.max, false, true},
        {int64(0), int64.max + 1, false, true},
        {int64(0), int64(-1), false, true},
        {int64.max, int64.max, true, false},
        {int64.max, int64.max + 1, false, true},
        {int64.max + 1, int64.max + 1, true, false},
        {int64(-2), int64(-2), true, false},
        {int64(-2), int64(-1), false, true},
    } do
        local a, b, eq, lt = table.unpack(test)
        local got = int64.ult(a, b)
        check(t, int64.ult, a, '(ult)', b, lt)
        check(t, int64.ult, b, '(ult)', a, not (lt or eq))
    end
end

function test_tostring(t)
    for _, test in ipairs{
        {int64(0), '0'},
        {int64(1234), '1234'},
        {int64(-1234), '-1234'},
        {int64.max, '9223372036854775807'},
        {int64.min, '-9223372036854775808'},
    } do
        local value, want = table.unpack(test)
        t:expect(t.expr.tostring(value)):eq(want)
    end
end

function test_concat(t)
    if not pcall(function() return 1 .. 2 end) then
        t:skip("automatic number => string conversions disabled")
    end

    local values = {-1, 3, -7, 13, -1234, 2468}
    local ops = {
        ['..'] = function(a, b) return a .. b end,
    }
    run_binary_ops_tests(t, ops, values, values)
end
