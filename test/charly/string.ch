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

export class StringTest {
    static func test_string_concat {
        const a = "hello "
        const b = "world"
        const c = a + b

        assert c == "hello world"
        assert c.length == 11

        assert ("" + "").length == 0
        assert ("" + "") == ""
    }

    static func test_string_length {
        assert "".length == 0
        assert "hello".length == 5
        assert "hello world my name is leonard".length == 30
        assert "ä".length == 1
        assert "πππ".length == 3
        assert "\n".length == 1
        assert "\n\n\n".length == 3
    }

    static func test_format_string {
        let a = 25
        let b = -988.18
        let c = true
        let d = null
        let e = "äää"
        let f = (1, 2, 3)
        let g = [4, 5, 6]
        let h = ""

        const i = "{a} {b} {c} {d} {e} {f} {g} {h}"
        assert i == "25 -988.18 true null äää (1, 2, 3) [4, 5, 6] "
    }

    static func test_string_multiply {
        const a = ""
        const b = "ä"
        const c = "test"

        assert a * 0 == ""
        assert a * 1 == ""
        assert a * 2 == ""
        assert a * 3 == ""

        assert b * 0 == ""
        assert b * 1 == "ä"
        assert b * 2 == "ää"
        assert b * 3 == "äää"

        assert c * 0 == ""
        assert c * 1 == "test"
        assert c * 2 == "testtest"
        assert c * 3 == "testtesttest"
    }

    static func test_string_comparison {
        const a = "test"
        const b = ""
        const c = "ää"
        const d = "hello world this is a test"

        assert a == "test"
        assert b == ""
        assert c == "ää"
        assert d == "hello world this is a test"

        assert a != "tes"
        assert b != "a"
        assert c != "ä"
        assert d != "hello world this is a tes"
    }

    static func test_string_indexing {
        const a = "hello äääπππ"

        assert a[0] == "h"
        assert a[1] == "e"
        assert a[2] == "l"
        assert a[3] == "l"
        assert a[4] == "o"
        assert a[5] == " "
        assert a[6] == "ä"
        assert a[7] == "ä"
        assert a[8] == "ä"
        assert a[9] == "π"
        assert a[10] == "π"
        assert a[11] == "π"

        assert a[-1] == "π"
        assert a[-2] == "π"
        assert a[-3] == "π"
        assert a[-4] == "ä"
        assert a[-5] == "ä"
        assert a[-6] == "ä"
        assert a[-7] == " "
        assert a[-8] == "o"
        assert a[-9] == "l"
        assert a[-10] == "l"
        assert a[-11] == "e"
        assert a[-12] == "h"
    }

    static func test_string_out_of_bounds_exception {
        const a = "test"

        const exc1 = assert_throws(->a[4])
        const exc2 = assert_throws(->a[-5])

        assert exc1.message == "String index 4 is out of range"
        assert exc2.message == "String index -5 is out of range"
    }

    static func test_string_spread {
        const a = "testäää"
        const t = (...a)
        const l = [...a]

        assert t.length == 7
        assert t == ("t", "e", "s", "t", "ä", "ä", "ä")

        assert l.length == 7
        assert l == ["t", "e", "s", "t", "ä", "ä", "ä"]
    }

    static func test_string_stdlib_begins_with {
        const a = "hello world"

        assert a.begins_with("hello")
        assert a.begins_with("hello world")
        assert a.begins_with("h")
        assert a.begins_with("")

        assert a.begins_with("test") == false
        assert a.begins_with("hello world test") == false
        assert a.begins_with("a") == false
        assert a.begins_with("world") == false
    }

    static func test_string_max_size_exceeded_exception {
        const string_one_mb = "a" * (1024 * 1024)
        const exc2 = assert_throws(->string_one_mb * (1024 * 32))
        assert exc2.message == "String exceeds maximum allowed size"

        // strings could also exceed their max size through string addition or format strings
        // however testing this here would cause the test suite to run for a very long time
        // which is why it is not being tested here
    }
}





