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

export class FunctionTest {
    static func test_function {
        func multiply_and_add(x, y, z) {
            const tmp = x * y
            return tmp + z
        }

        assert multiply_and_add(1, 2, 3) == 5
        assert multiply_and_add(0, 10, 30) == 30
        assert multiply_and_add(-4, 8, -9) == -41
        assert multiply_and_add(3, 4, 8) == 20
    }

    static func test_function_as_value {
        const f = func do_stuff(x) {
            return x * x
        }

        assert f(1) == 1
        assert f(2) == 4
        assert f(3) == 9
        assert f(4) == 16
    }

    static func test_function_implicit_return {
        func foo(x, y) {
            x + y
        }

        assert foo(1, 2) == 3
        assert foo(3, 4) == 7
        assert foo(5, 6) == 11
        assert foo(7, 8) == 15

        func bar(x) {
            if x {
                100
            } else {
                200
            }
        }

        assert bar(true) == null
        assert bar(false) == null
    }

    static func test_function_omit_parens {
        func foo {
            return 200
        }

        assert foo() == 200
    }

    static func test_function_expression_notation {
        func foo = 100
        func bar = foo() + 1
        func baz = bar() + 1

        assert foo() == 100
        assert bar() == 101
        assert baz() == 102
    }

    static func test_function_closures {
        func create_counter {
            let counter = 0

            return func get {
                const tmp = counter
                counter += 1
                tmp
            }
        }

        const counter = create_counter()

        assert counter() == 0
        assert counter() == 1
        assert counter() == 2
        assert counter() == 3
    }

    static func test_function_multiple_closures {
        let a = 0
        func foo {
            func bar {
                func baz {
                    a += 1
                }
                baz()
                baz()
                baz()
            }
            bar()
            bar()
            bar()
        }

        foo()
        foo()
        foo()

        assert a == 27
    }

    static func test_function_callback {
        func invoke_callback(callback) {
            callback() + 1
        }

        assert invoke_callback(->100) == 101
        assert invoke_callback(->0) == 1
        assert invoke_callback(->-10) == -9
    }

    static func test_function_consecutive_calls {
        func foo {
            func bar {
                func baz {
                    200
                }
            }
        }

        const result = foo()()()
        assert result == 200
    }

    static func test_function_not_enough_argc {
        func foo(x, y, z) = x + y + z

        const exc1 = assert_throws(->foo())
        const exc2 = assert_throws(->foo(1))
        const exc3 = assert_throws(->foo(1, 2))

        assert exc1.message == "Not enough arguments for function call, expected 3 but got 0"
        assert exc2.message == "Not enough arguments for function call, expected 3 but got 1"
        assert exc3.message == "Not enough arguments for function call, expected 3 but got 2"
    }

    static func test_function_too_many_argc {
        func foo(x, y, z) = x + y + z

        const exc1 = assert_throws(->foo(1, 2, 3, 4))
        const exc2 = assert_throws(->foo(1, 2, 3, 4, 5))
        const exc3 = assert_throws(->foo(1, 2, 3, 4, 5, 6))

        assert exc1.message == "Too many arguments for function call, expected at most 3 but got 4"
        assert exc2.message == "Too many arguments for function call, expected at most 3 but got 5"
        assert exc3.message == "Too many arguments for function call, expected at most 3 but got 6"
    }

    static func test_function_spread_argument {
        func foo(x, y, ...rest) {
            return (x, y, rest)
        }

        const (x, y, rest) = foo(1, 2, 3, 4, 5, 6)

        assert x == 1
        assert y == 2
        assert rest == (3, 4, 5, 6)

        func bar(...rest) = rest

        assert bar(1, 2, 3, 4, 5, 6) == (1, 2, 3, 4, 5, 6)
    }

    static func test_function_default_arguments {
        func foo(x = 1, y = 2, z = 3) = (x, y, z)

        assert foo() == (1, 2, 3)
        assert foo(100) == (100, 2, 3)
        assert foo(100, 200) == (100, 200, 3)
        assert foo(100, 200, 300) == (100, 200, 300)
    }

    static func test_function_default_arguments_order {
        let counter = 0
        let order = []
        func n(i) {
            order.push(i)
            const tmp = counter
            counter += 1
            tmp
        }

        func foo(x = n(1), y = n(2), z = n(3)) = (x, y, z)

        assert foo() == (0, 1, 2)
        assert foo(10) == (10, 3, 4)
        assert foo(10, 20) == (10, 20, 5)
        assert foo(10, 20, 30) == (10, 20, 30)

        assert counter == 6
        assert order == [1, 2, 3, 2, 3, 3]
    }

    static func test_function_default_arguments_references_others {
        func foo(x = 1, y = x + 1, z = y + 1) = (x, y, z)

        assert foo() == (1, 2, 3)
        assert foo(10) == (10, 11, 12)
        assert foo(10, 20) == (10, 20, 21)
        assert foo(10, 20, 30) == (10, 20, 30)
    }

    static func test_function_self_from_call {
        class A {
            func foo = self
        }

        const a = A()
        const r = a.foo()

        assert r == a
    }

    static func test_function_arrow_function {
        const foo = ->(x, y) {
            return x + y
        }

        assert foo(1, 2) == 3
        assert foo(5, 10) == 15
    }

    static func test_function_arrow_omit_parens {
        const foo = ->{
            return 100
        }

        assert foo() == 100
    }

    static func test_function_arrow_omit_block {
        const foo = ->(x, y) x + y

        assert foo(1, 2) == 3
        assert foo(100, 100) == 200
    }

    static func test_function_arrow_function_self_propagation {
        class A {
            func foo {
                return ->{
                    return self
                }
            }

            func bar {
                return func get {
                    return self
                }
            }
        }

        const a = A()

        const f = a.foo()
        assert f() == a

        const b = a.bar()
        assert b() == null
    }

    static func test_function_arrow_farself_propagation {
        class A {
            property value

            func foo {
                let a = value
                return ->{
                    let b = a
                    return ->{
                        let c = b
                        return c
                    }
                }
            }
        }

        const a = A(100)
        assert a.foo()()() == 100
    }

    static func test_function_arrow_function_not_enough_argc {
        const foo = ->(x, y, z) x + y + z

        const exc1 = assert_throws(->foo())
        const exc2 = assert_throws(->foo(1))
        const exc3 = assert_throws(->foo(1, 2))

        assert exc1.message == "Not enough arguments for function call, expected 3 but got 0"
        assert exc2.message == "Not enough arguments for function call, expected 3 but got 1"
        assert exc3.message == "Not enough arguments for function call, expected 3 but got 2"
    }

    static func test_function_arrow_function_excess_argc {
        const foo = ->(x, y, z) x + y + z

        assert_no_exception(->foo(1, 2, 3, 4))
        assert_no_exception(->foo(1, 2, 3, 4, 5))
        assert_no_exception(->foo(1, 2, 3, 4, 5, 6))
    }

    static func test_function_arrow_function_spread {
        const foo = ->(x, y, ...rest) (x, y, rest)

        assert foo(1, 2) == (1, 2, ())
        assert foo(1, 2, 3) == (1, 2, (3,))
        assert foo(1, 2, 3, 4) == (1, 2, (3, 4))
        assert foo(1, 2, 3, 4, 5) == (1, 2, (3, 4, 5))
    }

    static func test_function_non_callable_value {
        const a = 25
        const b = "hello world"
        const c = false
        const d = (1, 2, 3)
        const e = [1, 2, 3]

        const exc1 = assert_throws(->a())
        const exc2 = assert_throws(->b())
        const exc3 = assert_throws(->c())
        const exc4 = assert_throws(->d())
        const exc5 = assert_throws(->e())

        assert exc1.message == "Cannot call value of type 'Int'"
        assert exc2.message == "Cannot call value of type 'String'"
        assert exc3.message == "Cannot call value of type 'Bool'"
        assert exc4.message == "Cannot call value of type 'Tuple'"
        assert exc5.message == "Cannot call value of type 'List'"
    }

    static func test_function_builtin_incorrect_argc {
        const exc = assert_throws(->builtin_exit())
        const msg = "Incorrect argument count for builtin function 'charly.builtin.core.exit', expected 1 but got 0"
        assert exc.message == msg
    }

    static func test_function_class_member_initializer_argument {
        class A {
            property value
            func foo(@value)
        }

        const a = A()

        assert a.value == null

        a.foo(100)

        assert a.value == 100
    }

    static func test_function_call_spread {
        func foo(...args) = args

        const a = (1, 2, 3)
        const b = [1, 2, 3]
        const c = "abc"

        assert foo(...a) == (1, 2, 3)
        assert foo(...b) == (1, 2, 3)
        assert foo(...c) == ("a", "b", "c")
    }

    static func test_function_recursion_depth_limit {
        func foo {
            foo()
        }

        const exc = assert_throws(->foo())
        assert exc.message == "Maximum recursion depth exceeded"
    }
}





