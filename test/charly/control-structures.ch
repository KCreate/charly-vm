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

import { assert_throws } from unittest

export class ControlStructuresTest {
    static func test_control_if {
        let cmp
        let check

        check = true
        if check {
            cmp = "then"
        }
        assert cmp == "then"

        check = false
        cmp = null
        if check {
            cmp = "then"
        }
        assert cmp == null

        check = true
        cmp = null
        if check {
            cmp = "then"
        } else {
            cmp = "else"
        }
        assert cmp == "then"

        check = false
        cmp = null
        if check {
            cmp = "then"
        } else {
            cmp = "else"
        }
        assert cmp == "else"
    }

    static func test_control_while_entered {
        let i = 0
        let sum = 0
        while i < 10 {
            sum += i
            i += 1
        }
        assert i == 10
        assert sum == 45
    }

    static func test_control_while_skipped {
        let i = 100
        let sum = 0
        while i == 0 {
            sum += 1000
            i += 1000
        }
        assert i == 100
        assert sum == 0
    }

    static func test_control_while {
        let i = 0
        while i < 100 {
            if i == 50 {
                break
            }

            i += 1
        }

        assert i == 50
    }

    static func test_control_loop  {
        let i = 0

        loop {
            if i == 10 {
                break
            }

            i += 1
        }

        assert i == 10
    }

    static func test_control_break_scope_specific  {
        let i = 0
        let j = 0
        let c = 0

        while i < 100 {
            j = 0

            while j < 100 {
                if j == 75 break
                j += 1
                c += 1
            }

            i += 1

            if i == 50 break
        }

        assert i == 50
        assert j == 75
        assert c == 3750
    }

    static func test_control_continue  {
        let i = 0
        let c = 0

        while i < 10 {
            if i < 5 {
                i += 1
                continue
            }

            c += 1
            i += 1
        }

        assert i == 10
        assert c == 5
    }

    static func test_control_continue_scope_specific  {
        let i = 0
        let c = 0

        while i < 100 {
            if i < 50 {
                c += 1
                i += 1
                continue
            }

            i += 1
        }

        assert i == 100
        assert c == 50
    }

    static func test_control_switch  {
        func foo(c) {
            switch c {
                case 0 {
                    return "c is 0"
                }
                case 20 {
                    return "c is 20"
                }
                case 50 {
                    return "c is 50"
                }
                default {
                    return "c is something else"
                }
            }
        }

        assert foo(0) == "c is 0"
        assert foo(20) == "c is 20"
        assert foo(50) == "c is 50"
        assert foo(-20) == "c is something else"
        assert foo(-50) == "c is something else"
        assert foo(1) == "c is something else"
        assert foo(2) == "c is something else"
        assert foo(3) == "c is something else"
        assert foo(4) == "c is something else"
    }

    static func test_control_switch_break  {
        let c = 0

        func foo(i) {
            switch i {
                default {
                    if i < 50 {
                        break
                    }

                    c += 1
                }
            }
        }

        foo(0)
        foo(20)
        foo(40)
        foo(60)
        foo(80)
        foo(100)

        assert c == 3
    }

    static func test_control_switch_inside_loop {
        const ops = (("load", 20), ("load", 10), ("add", null), ("ret", null))

        let i = 0
        let result
        const stack = []
        while i < ops.length {
            const (name, arg) = ops[i]
            i += 1

            switch name {
                case "load" {
                    stack.push(arg)
                    continue
                }
                case "add" {
                    const r = stack.pop()
                    const l = stack.pop()
                    stack.push(l + r)
                    continue
                }
                case "ret" {
                    result = stack.pop()
                }
            }

            break
        }

        assert result == 30
        assert stack.length == 0
        assert i == ops.length
    }

    static func test_control_defer {
        let c
        const f1 = ->{
            defer {
                c = 100
            }

            return 500
        }

        assert f1() == 500
        assert c == 100

        c = null
        const f2 = ->{
            defer c = "test"
            throw "some error"
        }
        const exc = assert_throws(->f2())
        assert exc.message == "some error"
        assert c == "test"

        c = null
        const f3 = ->{
            loop {
                defer c = "hello world"
                break
            }
        }

        f3()

        assert c == "hello world"
    }
}





