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

export class ExceptionTest {
    static func test_exception_throw {
        const exc = assert_throws(->{
            throw "hello world"
        })

        assert exc.message == "hello world"
    }

    static func test_exception_throw_instance {
        let e = null

        const exc = assert_throws(->{
            e = Exception("some error message")
            throw e
        })

        assert exc == e
        assert exc.message == "some error message"
    }

    static func test_exception_catch {
        let exc = null

        assert_no_exception(->{
            try {
                throw "hello world"
            } catch(e) {
                exc = e
                return
            }

            assert false : "Unreachable"
        })

        assert exc instanceof Exception
        assert exc.message == "hello world"
    }

    static func test_exception_catch_default_name {
        const exc = assert_throws(->{
            let tmp
            try {
                throw "hello world"
            } catch {
                tmp = error
            }

            throw tmp
        })

        assert exc instanceof Exception
        assert exc.message == "hello world"
    }

    static func test_exception_try_expression_syntax {
        const exc = assert_throws(->{
            func foo = throw "hello world"
            let tmp
            try tmp = foo()
            catch tmp = "some other error"
            throw tmp
        })

        assert exc instanceof Exception
        assert exc.message == "some other error"
    }

    static func test_exception_catch_bubble_up {
        func foo = throw "hello world"
        func bar = foo()
        func baz = bar()
        func quz = baz()

        const exc = assert_throws(->quz())
        assert exc.message == "hello world"
    }

    static func test_exception_finally {
        let a = null

        try {
            a = 100
        } finally {
            a = 200
        }

        assert a == 200
    }

    static func test_exception_finally_throw {
        let a = null

        const exc = assert_throws(->{
            try {
                throw "hello world"
            } finally {
                a = 100
            }
        })

        assert exc.message == "hello world"
        assert a == 100
    }

    static func test_exception_finally_return {
        let a = null

        func foo {
            try {
                return "return value"
            } finally {
                a = 100
            }
        }

        assert foo() == "return value"
        assert a == 100
    }

    static func test_exception_finally_break {
        let a = null

        loop {
            try {
                break
            } finally {
                a = 100
            }

            assert false : "Unreachable"
        }

        assert a == 100
    }

    static func test_exception_finally_continue {
        let a = null

        let condition = true
        while condition {
            try {
                condition = false
                continue
            } finally {
                a = 100
            }

            assert false : "Unreachable"
        }

        assert condition == false
        assert a == 100
    }

    static func test_exception_finally_bubble_up {
        let a = null
        let b = null

        func foo = throw "hello world"
        func bar {
            try foo() finally {
                a = 100
            }
        }
        func baz {
            try bar() finally {
                b = 200
            }
        }

        const exc = assert_throws(->baz())

        assert exc.message == "hello world"
        assert a == 100
        assert b == 200
    }

    static func test_exception_catch_finally {
        let a = null
        let b = null

        func foo(cond) {
            try {
                if cond {
                    throw "hello world"
                }
            } catch(e) {
                throw e
            } finally {
                a = 100
            }
        }

        const exc = assert_throws(->foo(true))
        assert exc.message == "hello world"
        assert a == 100

        a = null

        assert_no_exception(->foo(false))
        assert a == 100
    }

    static func test_exception_cause {
        const exc = assert_throws(->{
            try {
                throw "hello world"
            } catch(e) {
                throw "some other error"
            }
        })

        assert exc.message == "some other error"
        assert exc.cause instanceof Exception
        assert exc.cause.message == "hello world"
    }

    static func test_exception_nested_try_cause {
        const exc = assert_throws(->{
            try {
                try {
                    throw "error1"
                } catch(e) {
                    throw "error2"
                }
            } catch(e) {
                throw "error3"
            }
        })

        assert exc.message == "error3"
        assert exc.cause instanceof Exception
        assert exc.cause.message == "error2"
        assert exc.cause.cause instanceof Exception
        assert exc.cause.cause.message == "error1"
    }

    static func test_exception_nested_catch_cause {
        const exc = assert_throws(->{
            try {
                try {
                    throw "error1"
                } catch(e) {
                    throw "error2"
                }
            } catch(e) {
                try {
                    throw "error3"
                } catch(e) {
                    throw "error4"
                }
            }
        })

        assert exc.message == "error4"
        assert exc.cause.message == "error3"
        assert exc.cause.cause.message == "error2"
        assert exc.cause.cause.cause.message == "error1"
    }

    static func test_exception_custom_exception_class {
        class MyCustomException extends Exception {
            property foobar
            func constructor(message, @foobar) = super(message)
        }

        const exc = assert_throws(->throw MyCustomException("hello world", 1234))

        assert exc instanceof MyCustomException
        assert exc instanceof Exception

        assert exc.message == "hello world"
        assert exc.foobar == 1234
    }

    static func test_exception_only_string_or_instance {
        const exc = assert_throws(->{
            throw 100
        })

        assert exc.message == "Expected thrown value to be an exception or a string"
    }
}





