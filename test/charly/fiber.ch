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

export class FiberTest {
    static func test_fiber {
        const a = spawn {
            return 100
        }

        const b = spawn {
            return await a + 100
        }

        const c = spawn {
            return await b + 100
        }

        assert await c == 300
    }

    static func test_fiber_throw {
        const a = spawn {
            throw "some error"
        }

        const b = spawn {
            await a
        }

        const c = spawn {
            await b
        }

        assert assert_throws(->await a).message == "some error"
        assert assert_throws(->await b).message == "some error"
        assert assert_throws(->await c).message == "some error"

        assert assert_throws(->await a).message == "some error"
        assert assert_throws(->await b).message == "some error"
        assert assert_throws(->await c).message == "some error"
    }

    static func test_fiber_spawn_call {
        let mem = []
        func add(x) = mem.push(x)

        class A {
            property mem = []
            func add(value) = mem.push(value)
        }

        const a = A()

        const tasks = [
            spawn add(2),
            spawn add(2),
            spawn add(2),
            spawn add(2),
            spawn a.add(8),
            spawn a.add(8),
            spawn a.add(8),
            spawn a.add(8)
        ]

        tasks.each(->(t) await t)

        mem = mem.reduce(0, ->(p, n) p + n)
        a.mem = a.mem.reduce(0, ->(p, n) p + n)

        assert mem == 8
        assert a.mem == 32
    }

    static func test_fiber_await_same_multiple_times {
        const fut = Future.create()

        const a = spawn await fut

        const tasks = [
            spawn await a,
            spawn await a,
            spawn await a,
            spawn await a
        ]

        spawn {
            fut.resolve("done")
        }

        const r = tasks.map(->(t) await t)

        assert r.length == 4
        assert r == ["done", "done", "done", "done"]
    }

    static func test_fiber_current {
        const n = 1000
        func foo = Fiber.current()

        const tasks = List.create_with(n, ->(i) spawn foo())
        const results = tasks.map(->(t) await t)

        assert tasks == results
    }
}












