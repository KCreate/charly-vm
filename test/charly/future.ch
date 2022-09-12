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

export class FutureTest {
    static func test_future_with_callback {
        const f = Future.create(->{
            return 100
        })

        assert await f == 100

        const g = Future.create(->{
            throw "some error"
        })

        assert assert_throws(->await g).message == "some error"
    }

    static func test_future_without_callback {
        const f = Future.create()
        const g = Future.create()
        const h = Future.create()

        const i = Future.create(->{
            f.resolve("f done")
            g.resolve("g done")
            h.resolve("h done")

            throw "error"
        })

        assert await f == "f done"
        assert await g == "g done"
        assert await h == "h done"

        assert assert_throws(->await i).message == "error"
    }

    static func test_future_reject {
        const a = Future.create()
        a.reject("hello world")
        assert assert_throws(->await a).message == "hello world"

        const b = Future.create()
        b.reject(Exception("other error"))
        assert assert_throws(->await b).message == "other error"

        const c = Future.create()
        c.reject(100)
        assert assert_throws(->await c).message == "Expected thrown value to be an exception or a string"
    }

    static func test_future_resolve_reject_again {
        const a = Future.create()
        a.resolve(100)
        assert await a == 100
        assert assert_throws(->a.resolve(200)).message == "Future has already completed"
        assert assert_throws(->a.reject("error")).message == "Future has already completed"
        assert await a == 100

        const b = Future.create()
        b.reject("error")
        assert assert_throws(->await b).message == "error"
        assert assert_throws(->b.resolve(200)).message == "Future has already completed"
        assert assert_throws(->b.reject("other error")).message == "Future has already completed"
        assert assert_throws(->await b).message == "error"
    }

    static func test_future_await_multiple_resolved {
        const a = Future.create()

        const tasks = Tuple.create_with(100, ->spawn await a)

        a.resolve(1)

        const sum = tasks.reduce(0, ->(p, n) p + await n)

        assert sum == 100
    }

    static func test_future_await_multiple_rejected {
        const a = Future.create()

        let n = 100

        const tasks = Tuple.create_with(n, ->spawn await a)

        a.reject("a")

        const exceptions = tasks.map(->(task) {
            try await task catch(e) {
                return e
            }
        })

        assert exceptions.length == n
        assert exceptions[0].message = "a"
        assert exceptions[n / 2].message = "a"
        assert exceptions[n - 1].message = "a"
    }
}












