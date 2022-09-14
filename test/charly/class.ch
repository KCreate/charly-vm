/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

import { assert_throws, assert_no_exception } from unittest

export class ClassTest {
    static func test_class {
        class A {}
        class B extends A {}
        class C extends B {}

        const a = A()
        const b = B()
        const c = C()

        assert a instanceof A
        assert b instanceof A
        assert c instanceof A
        assert a instanceof B == false
        assert b instanceof B
        assert c instanceof B
        assert a instanceof C == false
        assert b instanceof C == false
        assert c instanceof C
    }

    static func test_class_constructor {
        let called_count = 0

        class A {
            func constructor {
                called_count += 1
            }
        }

        10.times(->{
            A()
        })

        assert called_count == 10
    }

    static func test_class_default_constructors {
        class A {
            property foo
            property bar
            property baz
        }

        const a = A(1, 2, 3)

        assert a.foo == 1
        assert a.bar == 2
        assert a.baz == 3

        class B extends A {}

        const b = B(1, 2, 3)

        assert b.foo == 1
        assert b.bar == 2
        assert b.baz == 3
    }

    static func test_class_member_property_initializers {
        class A {
            property foo
            property bar
            property baz

            func constructor(@foo, @bar, @baz)
        }

        const a = A(1, 2, 3)

        assert a.foo == 1
        assert a.bar == 2
        assert a.baz == 3
    }

    static func test_class_super_constructor {
        class A {
            property foo
            property bar

            func constructor(@foo, @bar)
        }

        class B extends A {
            property baz
            property quz

            func constructor(foo, bar, @baz, @quz) = super(foo, bar)
        }

        class C extends B {
            property qaz
            property qiz

            func constructor(foo, bar, baz, quz, @qaz, @qiz) = super(foo, bar, baz, quz)
        }

        const a = A(1, 2)
        const b = B(1, 2, 3, 4)
        const c = C(1, 2, 3, 4, 5, 6)

        assert a.foo == 1
        assert a.bar == 2

        assert b.foo == 1
        assert b.bar == 2
        assert b.baz == 3
        assert b.quz == 4

        assert c.foo == 1
        assert c.bar == 2
        assert c.baz == 3
        assert c.quz == 4
        assert c.qaz == 5
        assert c.qiz == 6
    }

    static func test_class_super_method_calls {
        class A {
            func foo = "A::foo"
            func bar = "A::bar"
        }
        const a = A()
        assert a.foo.host_class == A
        assert a.bar.host_class == A
        assert a.foo() == "A::foo"
        assert a.bar() == "A::bar"

        class B extends A {
            func foo {
                const p = super()
                const o = super.bar()
                "B::foo, {p}, {o}"
            }

            func bar {
                const p = super()
                const o = super.foo()
                "B::bar, {p}, {o}"
            }
        }
        const b = B()
        assert b.foo.host_class == B
        assert b.bar.host_class == B
        assert b.foo() == "B::foo, A::foo, A::bar"
        assert b.bar() == "B::bar, A::bar, A::foo"

        class C extends B {}
        const c = C()
        assert c.foo.host_class == B
        assert c.bar.host_class == B
        assert c.foo() == "B::foo, A::foo, A::bar"
        assert c.bar() == "B::bar, A::bar, A::foo"



        const a2 = A()
        assert a2.foo.host_class == A
        assert a2.bar.host_class == A
        assert a2.foo() == "A::foo"
        assert a2.bar() == "A::bar"

        const b2 = B()
        assert b2.foo.host_class == B
        assert b2.bar.host_class == B
        assert b2.foo() == "B::foo, A::foo, A::bar"
        assert b2.bar() == "B::bar, A::bar, A::foo"

        const c2 = C()
        assert c2.foo.host_class == B
        assert c2.bar.host_class == B
        assert c2.foo() == "B::foo, A::foo, A::bar"
        assert c2.bar() == "B::bar, A::bar, A::foo"
    }

    static func test_class_super_overload_call {
        class A {
            func foo() = "A -"
            func foo(a) = "A {a}"
            func foo(a, b) = "A {a} {b}"
            func foo(a, b, c, ...rest) = "A {a} {b} {c} {rest}"
        }

        class B extends A {
            func foo() = "B {super()}"
            func foo(a) = "B {super(a)}"
            func foo(a, b) = "B {super(a, b)}"
            func foo(a, b, c, ...rest) = "B {super(a, b, c, ...rest)}"
        }

        const a = A()
        const b = B()

        assert a.foo() == "A -"
        assert a.foo(1) == "A 1"
        assert a.foo(1, 2) == "A 1 2"
        assert a.foo(1, 2, 3, 4, 5, 6) == "A 1 2 3 (4, 5, 6)"

        assert b.foo() == "B A -"
        assert b.foo(1) == "B A 1"
        assert b.foo(1, 2) == "B A 1 2"
        assert b.foo(1, 2, 3, 4, 5, 6) == "B A 1 2 3 (4, 5, 6)"
    }

    static func test_class_super_named_method_calls {
        class A {
            func foo = "A foo"
            func bar = "A bar"
        }

        class B extends A {
            func foo = "B foo {super.bar()}"
            func bar = "B bar {super.foo()}"
        }

        class C extends B {
            func foo = "C foo {super.bar()}"
            func bar = "C bar {super.foo()}"
        }

        const a = A()
        const b = B()
        const c = C()

        assert a.foo() == "A foo"
        assert a.bar() == "A bar"

        assert b.foo() == "B foo A bar"
        assert b.bar() == "B bar A foo"

        assert c.foo() == "C foo B bar A foo"
        assert c.bar() == "C bar B foo A bar"
    }

    static func test_class_property_read {
        class A {
            property foo
            property bar
            property baz
        }

        const a = A(1, 2, 3)

        assert a.foo == 1
        assert a.bar == 2
        assert a.baz == 3
    }

    static func test_class_property_write {
        class A {
            property foo
            property bar
            property baz
        }

        const a = A(1, 2, 3)

        assert a.foo == 1
        assert a.bar == 2
        assert a.baz == 3

        a.foo = 100
        a.bar = 200
        a.baz = 300

        assert a.foo == 100
        assert a.bar == 200
        assert a.baz == 300
    }

    static func test_class_function_lookup {
        class A {
            func foo = "A foo"
        }
        class B extends A {
            func bar = "B bar"
        }
        class C extends B {
            func baz = "C baz"
        }

        const a = A()
        const b = B()
        const c = C()

        assert a.foo() == "A foo"
        assert assert_throws(->a.bar()).message == "Object of type 'A' has no attribute 'bar'"
        assert assert_throws(->a.baz()).message == "Object of type 'A' has no attribute 'baz'"

        assert b.foo() == "A foo"
        assert b.bar() == "B bar"
        assert assert_throws(->b.baz()).message == "Object of type 'B' has no attribute 'baz'"

        assert c.foo() == "A foo"
        assert c.bar() == "B bar"
        assert c.baz() == "C baz"
    }

    static func test_class_function_self_argument {
        let a
        let b

        class A {
            property foo

            func test {
                a = self
                b = @foo
            }
        }

        const o = A(100)
        o.test()

        assert a == o
        assert b == 100
    }

    static func test_class_function_argument_self_assignment {
        class A {
            property foo
            property bar
            property baz

            func set(@foo, @bar, @baz)
        }

        const a = A()

        assert a.foo == null
        assert a.bar == null
        assert a.baz == null

        a.set(1, 2, 3)

        assert a.foo == 1
        assert a.bar == 2
        assert a.baz == 3
    }

    static func test_class_implicit_self {
        class A {
            property value

            func foo {
                value = 100
            }

            func bar {
                @value = 200
            }
        }

        const a = A()

        assert a.value == null

        a.foo()

        assert a.value == 100

        a.bar()

        assert a.value == 200
    }

    static func test_class_static_properties {
        class A {
            static property foo
            static property bar = 100
            static property baz = 200
        }

        assert A.foo == null
        assert A.bar == 100
        assert A.baz == 200

        A.foo = 4
        A.bar = 5
        A.baz = 6

        assert A.foo == 4
        assert A.bar == 5
        assert A.baz == 6

        assert assert_throws(->A.test1).message == "Object of type 'A' has no attribute 'test1'"
        assert assert_throws(->A.test2).message == "Object of type 'A' has no attribute 'test2'"
    }

    static func test_class_static_functions {
        class A {
            static func foo = 100
        }

        class B extends A {
            static func bar = 200
        }

        assert A.foo() == 100
        assert B.bar() == 200

        assert assert_throws(->A.bar()).message == "Object of type 'A' has no attribute 'bar'"
        assert assert_throws(->B.foo()).message == "Object of type 'B' has no attribute 'foo'"
    }

    static func test_class_private_properties {
        class A {
            property backup
            private property counter = 0

            func get {
                const tmp = counter
                counter += 1
                tmp
            }
        }

        const a = A()

        assert a.get() == 0
        assert a.get() == 1
        assert a.get() == 2
        assert a.get() == 3

        assert assert_throws(->a.counter).message == "Cannot read private property 'counter' of class 'A'"
        assert assert_throws(->a.counter = 0).message == "Cannot assign to private property 'counter' of class 'A'"
    }

    static func test_class_function_overloading {
        class A {
            func foo = "foo()"
            func foo(x) = "foo({x})"
            func foo(x, y) = "foo({x}, {y})"
            func foo(x, y, z, ...rest) = "foo({x}, {y}, {z}, ...{rest})"
        }

        const a = A()
        assert a.foo() == "foo()"
        assert a.foo(1) == "foo(1)"
        assert a.foo(1, 2) == "foo(1, 2)"
        assert a.foo(1, 2, 3) == "foo(1, 2, 3, ...())"
        assert a.foo(1, 2, 3, 4) == "foo(1, 2, 3, ...(4))"
        assert a.foo(1, 2, 3, 4, 5, 6, 7) == "foo(1, 2, 3, ...(4, 5, 6, 7))"
    }

    static func test_class_inheritance_properties {
        class A {
            property foo
            property bar
        }

        class B extends A {
            property baz
            property quz
            func constructor(foo, bar, @baz, @quz) = super(foo, bar)
        }

        class C extends B {
            property qaz
            property qiz
            func constructor(foo, bar, baz, quz, @qaz, @qiz) = super(foo, bar, baz, quz)
        }

        const a = A(1, 2)
        const b = B(3, 4, 5, 6)
        const c = C(7, 8, 9, 10, 11, 12)

        assert a.foo == 1
        assert a.bar == 2

        assert b.foo == 3
        assert b.bar == 4
        assert b.baz == 5
        assert b.quz == 6

        assert c.foo == 7
        assert c.bar == 8
        assert c.baz == 9
        assert c.quz == 10
        assert c.qaz == 11
        assert c.qiz == 12

        assert assert_throws(->a.baz = null).message == "Object of type 'A' has no writeable attribute 'baz'"
        assert assert_throws(->b.qaz = null).message == "Object of type 'B' has no writeable attribute 'qaz'"
    }

    static func test_class_inheritance_functions {
        class A {
            func foo = "A"
            func bar = "A"
            func baz = "A"
        }

        class B extends A {
            func foo = "B"
        }

        class C extends B {
            func bar = "C"
        }

        class D extends C {
            func baz = "D"
        }

        const a = A()
        const b = B()
        const c = C()
        const d = D()

        assert a.foo() == "A"
        assert a.bar() == "A"
        assert a.baz() == "A"

        assert b.foo() == "B"
        assert b.bar() == "A"
        assert b.baz() == "A"

        assert c.foo() == "B"
        assert c.bar() == "C"
        assert c.baz() == "A"

        assert d.foo() == "B"
        assert d.bar() == "C"
        assert d.baz() == "D"
    }

    static func test_class_inheritance_merge_overloads {
        class A {
            func foo = "foo()"
            func foo(x, y) = "foo({x}, {y})"
        }

        class B extends A {
            func foo(x) = "foo({x})"
            func foo(x, y, z) = "foo({x}, {y}, {z})"
        }

        const a = A()
        const b = B()

        assert a.foo() == "foo()"
        assert a.foo(1, 2) == "foo(1, 2)"

        assert b.foo() == "foo()"
        assert b.foo(1) == "foo(1)"
        assert b.foo(1, 2) == "foo(1, 2)"
        assert b.foo(1, 2, 3) == "foo(1, 2, 3)"

        assert assert_throws(->a.foo(1)).message == "Not enough arguments for function call, expected 2 but got 1"
        assert assert_throws(->a.foo(1, 2, 3)).message == "Too many arguments for function call, expected at most 2 but got 3"
    }

    static func test_class_inheritance_overload_specific {
        class A {
            func foo(...args) = args
        }

        class B extends A {
            func foo(x, y, z) = "foo({x}, {y}, {z})"
        }

        const a = A()
        const b = B()

        assert a.foo(1) == (1,)
        assert a.foo(1, 2) == (1, 2)
        assert a.foo(1, 2, 3) == (1, 2, 3)
        assert a.foo(1, 2, 3, 4) == (1, 2, 3, 4)
        assert a.foo(1, 2, 3, 4, 5) == (1, 2, 3, 4, 5)

        assert b.foo(1) == (1,)
        assert b.foo(1, 2) == (1, 2)
        assert b.foo(1, 2, 3) == "foo(1, 2, 3)"
        assert b.foo(1, 2, 3, 4) == (1, 2, 3, 4)
        assert b.foo(1, 2, 3, 4, 5) == (1, 2, 3, 4, 5)
    }

    static func test_class_inheritance_overload_spread_all {
        class A {
            func foo = "A::foo()"
            func foo(x) = "A::foo({x})"
            func foo(x, y) = "A::foo({x}, {y})"
        }

        class B extends A {
            func foo(...args) = args
        }

        class C extends B {
            func foo(x) = "C::foo({x})"
        }

        class D extends C {
            func foo(...args) = args
        }

        const a = A()
        const b = B()
        const c = C()
        const d = D()

        assert a.foo() == "A::foo()"
        assert a.foo(1) == "A::foo(1)"
        assert a.foo(1, 2) == "A::foo(1, 2)"

        assert b.foo() == ()
        assert b.foo(1) == (1,)
        assert b.foo(1, 2) == (1, 2)

        assert c.foo() == ()
        assert c.foo(1) == "C::foo(1)"
        assert c.foo(1, 2) == (1, 2)

        assert d.foo() == ()
        assert d.foo(1) == (1,)
        assert d.foo(1, 2) == (1, 2)
    }

    static func test_class_no_static_inheritance {
        class A {
            static property foo = 10
            static func bar = 20
        }

        class B extends A {}

        assert assert_throws(->B.foo).message == "Object of type 'Class' has no attribute 'foo'"
        assert assert_throws(->B.bar()).message == "Object of type 'Class' has no attribute 'bar'"
    }

    static func test_class_non_constructable_builtin_classes {
        const exc1 = assert_throws(->{
            String()
        })
        assert exc1.message == "Cannot instantiate class 'String'"

        const exc2 = assert_throws(->{
            AssertionException()
        })
        assert exc2.message == "Cannot instantiate class 'AssertionException'"
    }

    static func test_class_non_extendable_builtin_classes {
        const exc1 = assert_throws(->{
            class A extends String {}
        })
        assert exc1.message == "Cannot extend final class 'String'"

        const exc2 = assert_throws(->{
            class A extends AssertionException {}
        })
        assert exc2.message == "Cannot extend final class 'AssertionException'"
    }

    static func test_class_object_unpack {
        class A {
            property foo
            property bar
            property baz
        }

        const a = A(1, 2, 3)

        if true {
            const { foo, bar, baz } = a
            assert foo == 1
            assert bar == 2
            assert baz == 3
        }

        if true {
            const { baz, bar, foo } = a
            assert foo == 1
            assert bar == 2
            assert baz == 3
        }

        if true {
            const { baz, foo, bar } = a
            assert foo == 1
            assert bar == 2
            assert baz == 3
        }

        if true {
            const { baz, foo, bar } = a
            assert foo == 1
            assert bar == 2
            assert baz == 3
        }

        const exc1 = assert_throws(->{
            const { nonexistentproperty } = a
        })
        assert exc1.message == "Object of type 'A' has no attribute 'nonexistentproperty'"

        const exc2 = assert_throws(->{
            const { fail1, fail2, fail3 } = a
        })
        assert exc2.message == "Object of type 'A' has no attribute 'fail1'"

        const exc3 = assert_throws(->{
            const { fail3, fail2, fail1 } = a
        })
        assert exc3.message == "Object of type 'A' has no attribute 'fail3'"

        let foo
        let bar
        let fail
        const exc4 = assert_throws(->{
            { foo, bar, fail } = a
        })
        assert foo == null
        assert bar == null
        assert exc4.message == "Object of type 'A' has no attribute 'fail'"

        if true {
            const { length, klass } = [1, 2, 3, 4]
            assert length == 4
            assert klass == List
        }
    }
}

















