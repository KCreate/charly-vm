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

export class ListTest {
    static func test_list {
        const a = []
        const b = [1]
        const c = [1, 2, 3, 4, 5]

        assert a.length == 0
        assert b.length == 1
        assert c.length == 5

        assert a == []
        assert b == [1]
        assert c == [1, 2, 3, 4, 5]
    }

    static func test_list_create {
        const a = List.create(0)
        assert a.length == 0
        assert a == []

        const b = List.create(5)
        assert b.length == 5
        assert b == [null, null, null, null, null]

        const c = List.create(5, 100)
        assert c.length == 5
        assert c == [100, 100, 100, 100, 100]

        const d = List.create(8192, 12345678)
        assert d.length == 8192
        assert d[0] == 12345678
        assert d[4096] == 12345678
        assert d[8191] == 12345678
        assert d[-1] == 12345678

        const exc1 = assert_throws(->List.create(-10))
        assert exc1.message == "Expected length to be positive, got -10"
    }

    static func test_list_create_with {
        const a = List.create_with(10, ->(i) i)
        assert a.length == 10
        assert a == [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

        let called = false
        const b = List.create_with(0, ->called = true)
        assert called == false
        assert b == []

        const exc = assert_throws(->List.create_with(-10, ->{}))
        assert exc.message == "Expected length to be positive, got -10"
    }

    static func test_list_insert {
        const a = []
        a.insert(0, 1)
        a.insert(0, 2)
        a.insert(0, 3)
        assert a.length == 3
        assert a == [3, 2, 1]

        const b = [1, 2, 3, 4]
        b.insert(2, 100)
        b.insert(0, 200)
        b.insert(1, 300)
        assert b.length == 7
        assert b == [200, 300, 1, 2, 100, 3, 4]

        const c = [1, 2, 3]
        c.insert(c.length, 4)
        c.insert(c.length, 5)
        c.insert(c.length, 6)
        assert c.length == 6
        assert c == [1, 2, 3, 4, 5, 6]

        const d = [1, 2, 3]
        const exc1 = assert_throws(->d.insert(4, 100))
        const exc2 = assert_throws(->d.insert(-4, 100))
        assert exc1.message == "List index 4 is out of range"
        assert exc2.message == "List index -4 is out of range"
    }

    static func test_list_erase {
        let a = [1, 2, 3, 4]

        a.erase(0)
        assert a.length == 3
        assert a == [2, 3, 4]

        a.erase(1)
        assert a.length == 2
        assert a == [2, 4]

        a.erase(0, 2)
        assert a.length == 0
        assert a == []

        a = [1, 2, 3, 4, 5, 6, 7, 8]

        a.erase(2, 4)
        assert a.length == 4
        assert a == [1, 2, 7, 8]

        a.erase(2, 2)
        assert a.length == 2
        assert a == [1, 2]

        a.erase(0, 0)
        assert a.length == 2
        assert a == [1, 2]

        a.erase(0, 2)
        assert a.length == 0
        assert a == []

        a = [1, 2, 3, 4]
        const exc1 = assert_throws(->a.erase(-10))
        const exc2 = assert_throws(->a.erase(4))
        const exc3 = assert_throws(->a.erase(0, -10))
        const exc4 = assert_throws(->a.erase(2, 4))

        assert exc1.message == "List index -10 is out of range"
        assert exc2.message == "List index 4 is out of range"
        assert exc3.message == "Expected count to be greater than 0"
        assert exc4.message == "Not enough values in list to erase"
    }

    static func test_list_push {
        const a = []

        a.push(1)
        a.push(2)
        a.push(3)
        a.push(4)
        assert a.length == 4
        assert a == [1, 2, 3, 4]

        a.push(1)
        a.push(2)
        a.push(3)
        assert a.length == 7
        assert a == [1, 2, 3, 4, 1, 2, 3]
    }

    static func test_list_pop {
        const l = [1, 2, 3, 4]

        assert l.pop() == 4
        assert l.pop() == 3

        assert l.length == 2
        assert l == [1, 2]

        assert l.pop() == 2
        assert l.pop() == 1

        const exc = assert_throws(->l.pop())
        assert exc.message == "List is empty"
    }

    static func test_list_each {
        const l = [100, 200, 300, 400]
        const results = []

        l.each(->(...args) {
            results.push(args)
        })

        assert results.length == 4
        assert results[0] == (100, 0, l)
        assert results[1] == (200, 1, l)
        assert results[2] == (300, 2, l)
        assert results[3] == (400, 3, l)
    }

    static func test_list_each_concurrency_exception {
        const l = [1, 2, 3, 4]

        const exc = assert_throws(->{
            l.each(->(value, i) {
                l.pop()
            })
        })

        assert exc.message == "List size changed during iteration"
    }

    static func test_list_map {
        const l = [1, 2, 3, 4]
        const m = l.map(->(v, i) {
            v * v + i
        })

        assert l.length == 4
        assert l == [1, 2, 3, 4]

        assert m.length == 4
        assert m == [1, 5, 11, 19]
    }

    static func test_list_filter {
        const l = [1, 2, 3, 4, 5, 6, 7, 8]
        const m = l.filter(->(v) v > 4)

        assert l.length == 8
        assert l == [1, 2, 3, 4, 5, 6, 7, 8]

        assert m.length == 4
        assert m == [5, 6, 7, 8]

        const a = [1, 2, 3, 4]
        const arguments = []

        a.filter(->(...args) arguments.push(args))

        assert arguments.length == 4
        assert arguments == [(1, 0, a), (2, 1, a), (3, 2, a), (4, 3, a)]
    }

    static func test_list_contains {
        const a = ["a", "b", "c", "d", 100]

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

    static func test_list_sort {
        let a = []
        a.sort()

        assert a.length == 0

        a = [1, 9, 3, 0, -200, -25.1, 78, 3, 7]
        a.sort()

        assert a.length == 9
        assert a == [-200, -25.1, 0, 1, 3, 3, 7, 9, 78]

        a = [(25,), (-25,), (9,), (2,), (100,)]
        a.sort(->(left, right) left[0] <=> right[0])

        assert a.length == 5
        assert a == [(-25,), (2,), (9,), (25,), (100,)]
    }

    static func test_list_empty {
        assert [].empty()

        assert [1].empty() == false
        assert [1, 2].empty() == false
        assert [1, 2, 3].empty() == false
        assert [1, 2, 3, 4].empty() == false
    }

    static func test_list_copy {
        const a = [1, 2, 3, 4]
        const b = a.copy()

        assert b.length == 4
        assert b == [1, 2, 3, 4]

        a[0] = 100
        assert b[0] == 1

        b[1] = 200
        assert a[1] == 2
    }

    static func test_list_multiply {
        const a = []
        const b = [1]
        const c = [1, 2, 3]

        assert a * 0 == []
        assert b * 0 == []
        assert c * 0 == []

        assert a * 1 == []
        assert b * 1 == [1]
        assert c * 1 == [1, 2, 3]

        assert a * -5 == []
        assert b * -5 == []
        assert c * -5 == []

        assert a * 3 == []
        assert b * 3 == [1, 1, 1]
        assert c * 3 == [1, 2, 3, 1, 2, 3, 1, 2, 3]
    }

    static func test_list_comparison {
        const a = [[[1, 2, 3], 4, true, 6], [[[1, [1, "test", [[1, 2, null], 2, 3]], 3]]]]
        a.push(a)

        const b = a.copy()

        assert a == a
        assert a == b
        assert b == a
        assert b == b
    }

    static func test_list_comparison_recursion_exception {
        const a = [1]
        const b = [1]

        a.push(b)
        b.push(a)

        const exc = assert_throws(->a == b)
        assert exc.message == "Maximum recursion depth exceeded"
    }

    static func test_list_indexing {
        const a = [1, 2, 3, 4]

        assert a[0] == 1
        assert a[1] == 2
        assert a[2] == 3
        assert a[3] == 4

        assert a[-1] == 4
        assert a[-2] == 3
        assert a[-3] == 2
        assert a[-4] == 1

        const exc1 = assert_throws(->a[4])
        assert exc1.message == "List index 4 is out of range"

        const exc2 = assert_throws(->a[-5])
        assert exc2.message == "List index -5 is out of range"
    }

    static func test_list_write {
        const a = [1, 2, 3, 4]

        a[0] = 100
        a[1] = 200
        a[2] = 300
        a[3] = 400

        assert a == [100, 200, 300, 400]

        const exc1 = assert_throws(->a[4] = 1000)
        assert exc1.message == "List index 4 is out of range"

        const exc2 = assert_throws(->a[-5] = 1000)
        assert exc2.message == "List index -5 is out of range"
    }

    static func test_list_max_size_exceeded_exception {
        const list_max_capacity = 268435456
        const list_of_max_capacity = List.create(list_max_capacity)

        const exc1 = assert_throws(->list_of_max_capacity.push(1))
        assert exc1.message == "List exceeded max size"

        const exc2 = assert_throws(->list_of_max_capacity.insert(0, 100))
        assert exc2.message == "List exceeded max size"

        const exc3 = assert_throws(->List.create(list_max_capacity + 1000))
        assert exc3.message == "List exceeded max size"
    }
}





