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

export class TupleTest {
    static func test_tuple {
        const a = ()
        const b = (1,)
        const c = (1, 2, 3)

        assert b instanceof Tuple

        assert a.length == 0
        assert b.length == 1
        assert c.length == 3

        assert a == ()
        assert b == (1,)
        assert c == (1, 2, 3)
    }

    static func test_tuple_create {
        const a = Tuple.create(0, null)
        const b = Tuple.create(5, 100)
        const c = Tuple.create(1000, "hello")

        assert a.length == 0
        assert b.length == 5
        assert c.length == 1000

        assert a == ()
        assert b == (100, 100, 100, 100, 100)
        assert c[0] == "hello"
        assert c[500] == "hello"
        assert c[999] == "hello"

        const exc = assert_throws(->Tuple.create(-10, null))
        assert exc.message == "Expected length to be positive, got -10"
    }

    static func test_tuple_create_with {
        const a = Tuple.create_with(5, ->(i) i)
        assert a.length == 5
        assert a == (0, 1, 2, 3, 4)

        let called = false
        Tuple.create_with(0, ->called = true)
        assert called == false

        const exc = assert_throws(->Tuple.create_with(-10, ->called = true))
        assert exc.message == "Expected length to be positive, got -10"
        assert called == false
    }

    static func test_tuple_each {
        const a = (1, 2, 3, 4, 5)
        const args = []
        a.each(->(...a) args.push(a))

        assert args.length == 5
        assert args == [(1, 0, a), (2, 1, a), (3, 2, a), (4, 3, a), (5, 4, a)]
    }

    static func test_tuple_map {
        const a = (1, 2, 3, 4, 5)
        const b = a.map(->(v, i) v * v + i)

        assert b.length == 5
        assert b == (1, 5, 11, 19, 29)
    }

    static func test_tuple_filter {
        const a = (1, 2, 3, 4, 5, 4, 3, 2, 1)
        const b = a.filter(->(v) v < 4)

        assert b.length == 6
        assert b == (1, 2, 3, 3, 2, 1)

        const c = (1, 2, 3, 4)
        const args = []
        c.filter(->(...a) args.push(a))

        assert args.length == 4
        assert args == [(1, 0, c), (2, 1, c), (3, 2, c), (4, 3, c)]
    }

    static func test_tuple_reduce {
        const a = (1, 2, 3, 4, 5, 6)
        const product = a.reduce(->(p, n) p * n, 1)
        assert product == 720

        const args = []
        a.reduce(->(...a) {
            const (p, n, i, l) = a
            args.push(a)
            p * n
        }, 1)

        assert args.length == 6
        assert args == [
            (1, 1, 0, a),
            (1, 2, 1, a),
            (2, 3, 2, a),
            (6, 4, 3, a),
            (24, 5, 4, a),
            (120, 6, 5, a)
        ]
    }

    static func test_tuple_contains {
        const a = ("a", "b", "c", "d", 100)

        assert a.contains("a")
        assert a.contains("b")
        assert a.contains("c")
        assert a.contains("d")
        assert a.contains(100)

        assert a.contains("e") == false
        assert a.contains("f") == false
        assert a.contains("g") == false
        assert a.contains("hello world") == false
        assert a.contains(99) == false
        assert a.contains(true) == false
        assert a.contains(false) == false
        assert a.contains(a) == false
        assert a.contains(null) == false
    }

    static func test_tuple_empty {
        assert ().empty()
        assert (1,).empty() == false
        assert (1, 2).empty() == false
        assert (1, 2, 3).empty() == false
        assert (1, 2, 3, 4).empty() == false
    }

    static func test_tuple_multiply {
        const a = ()
        const b = (1,)
        const c = (1, 2, 3)

        assert a * 0 == ()
        assert b * 0 == ()
        assert c * 0 == ()

        assert a * 1 == ()
        assert b * 1 == (1,)
        assert c * 1 == (1, 2, 3)

        assert a * -5 == ()
        assert b * -5 == ()
        assert c * -5 == ()

        assert a * 3 == ()
        assert b * 3 == (1, 1, 1)
        assert c * 3 == (1, 2, 3, 1, 2, 3, 1, 2, 3)
    }

    static func test_tuple_comparison {
        const a = (((1, 2, 3), 4, true, 6), (((1, (1, "test", ((1, 2, null), 2, 3)), 3))))
        const b = (((1, 2, 3), 4, true, 6), (((1, (1, "test", ((1, 2, null), 2, 3)), 3))))

        assert a == a
        assert a == b
        assert b == a
        assert b == b
    }

    static func test_tuple_indexing {
        const a = (1, 2, 3, 4)

        assert a[0] == 1
        assert a[1] == 2
        assert a[2] == 3
        assert a[3] == 4

        assert a[-1] == 4
        assert a[-2] == 3
        assert a[-3] == 2
        assert a[-4] == 1

        const exc1 = assert_throws(->a[4])
        assert exc1.message == "Tuple index 4 is out of range"

        const exc2 = assert_throws(->a[-5])
        assert exc2.message == "Tuple index -5 is out of range"
    }

    static func test_tuple_readonly {
        const a = (1, 2, 3, 4)

        const exc = assert_throws(->a[0] = 0)
        assert exc.message == "Cannot write to tuple"
    }

    static func test_tuple_spread {
        const a = (1, 2, 3, 4)
        const b = "abcäää"
        const c = [5, 6, 7, 8]
        const t = (true, ...a, false, ...b, null, ...c, 2000)

        assert t == (true, 1, 2, 3, 4, false, "a", "b", "c", "ä", "ä", "ä", null, 5, 6, 7, 8, 2000)
    }

    static func test_tuple_empty_spread {
        const a = ()
        const b = ""
        const c = []
        const t = (...a, ...b, ...c)
        assert t == ()
    }

    static func test_tuple_sequence_unpack {
        const (a, b, c, d) = (1, 2, 3, 4)
        assert a == 1
        assert b == 2
        assert c == 3
        assert d == 4

        const exc = assert_throws(->{
            const (a, b) = (1, 2, 3, 4)
        })
        assert exc.message == "Expected tuple to be of length 2, got 4"
    }

    static func test_tuple_spread_sequence_unpack {
        const (a, b, ...rest) = (1, 2, 3, 4)
        assert a == 1
        assert b == 2
        assert rest == (3, 4)

        const exc = assert_throws(->{
            const (a, b, c, d, ...rest) = (1, 2)
        })
        assert exc.message == "Tuple does not contain enough values to unpack"
    }

    static func test_tuple_max_size_exceeded_exception {
        const tuple_max_capacity = 65012
        const tuple_of_max_capacity = Tuple.create(tuple_max_capacity, null)

        const exc1 = assert_throws(->Tuple.create(tuple_max_capacity + 1000, null))
        assert exc1.message == "Tuple exceeded max size"

        const exc2 = assert_throws(->{ return (...tuple_of_max_capacity, ...tuple_of_max_capacity) })
        assert exc2.message == "Tuple exceeded max size"

        const exc3 = assert_throws(->Tuple.create_with(tuple_max_capacity + 1000, ->null))
        assert exc3.message == "Tuple exceeded max size"

        const exc4 = assert_throws(->tuple_of_max_capacity * 2)
        assert exc4.message == "Tuple exceeded max size"
    }
}





