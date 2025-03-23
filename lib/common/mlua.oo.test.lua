-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local oo = require 'mlua.oo'
local string = require 'string'

local A = oo.class('A')
A.cls_a = 1

function A:__init(a) self.a = a end
function A:method() return self.a end
function A:__add(other) return A(self.a + other.a) end
function A:__len() return self.a end

local B = oo.class('B', A)
B.cls_b = 2

function B:__init(a, b)
    A.__init(self, a)
    self.b = b
end

function test_class_tostring(t)
    local got = tostring(A)
    t:expect(got == 'class A', "tostring(A) = %q", got)
    local got = tostring(B)
    t:expect(got == 'class B', "tostring(B) = %q", got)
end

function test_instance_tostring(t)
    local a, b = A(10), B(20, 30)
    t:expect(tostring(a)):label("tostring(a)"):matches('^A: 0?x?[0-9a-fA-F]+$')
    t:expect(tostring(b)):label("tostring(b)"):matches('^B: 0?x?[0-9a-fA-F]+$')
end

function test_class_attributes(t)
    t:expect(A.__name == 'A', "A.__name = %s, want A", A.__name)
    t:expect(B.__name == 'B', "B.__name = %s, want B", B.__name)
    t:expect(A.__base == nil, "A.__base = %s, want nil", A.__base)
    t:expect(B.__base == A, "B.__base = %s, want %s", B.__base, A)
end

function test_instance_attributes(t)
    local a, b = A(10), B(20, 30)
    t:expect(a.cls_a == 1, "a.cls_a = %s, want 1", a.cls_a)
    t:expect(a.a == 10, "a.a = %s, want 10", a.a)
    t:expect(b.cls_a == 1, "b.cls_a = %s, want 1", b.cls_a)
    t:expect(b.cls_b == 2, "b.cls_b = %s, want 2", b.cls_b)
    t:expect(b.a == 20, "b.a = %s, want 20", b.a)
    t:expect(b.b == 30, "b.b = %s, want 30", b.b)
end

function test_methods(t)
    local got = A(12):method()
    t:expect(got == 12, "got %s, want 12", got)
    local got = B(34, 56):method()
    t:expect(got == 34, "got %s, want 34", got)
end

function test_metamethods(t)
    local got = A(1) + A(2) + A(3)
    t:expect(got.a == 6, "got.a = %s, want 6", got.a)
    local got = #B(4, 5)
    t:expect(got == 4, "got %s, want 4", got)
end

function test_issubclass(t)
    t:expect(oo.issubclass(A, A), "issubclass(A, A) = false")
    t:expect(oo.issubclass(B, B), "issubclass(B, B) = false")
    t:expect(not oo.issubclass(A, B), "issubclass(A, B) = true")
    t:expect(oo.issubclass(B, A), "issubclass(B, A) = false")
end

function test_isinstance(t)
    local a, b = A(10), B(20, 30)
    t:expect(oo.isinstance(a, A), "isinstance(a, A) = false")
    t:expect(oo.isinstance(b, B), "isinstance(b, B) = false")
    t:expect(not oo.isinstance(a, B), "isinstance(a, B) = true")
    t:expect(oo.isinstance(b, A), "isinstance(b, A) = false")
end
