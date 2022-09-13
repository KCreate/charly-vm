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

export class GlobalsTest {
    static func test_globals_argv {
        assert ARGV.length == 4
        assert ARGV[1] == "foo"
        assert ARGV[2] == "bar"
        assert ARGV[3] == "baz"
    }

    static func test_globals_charly_stdlib_path {
        assert CHARLY_STDLIB instanceof String
        assert CHARLY_STDLIB.begins_with("/")
    }

    static func test_builtin_types {
        assert Value instanceof Class
        assert Number instanceof Class
        assert Int instanceof Class
        assert Float instanceof Class
        assert Bool instanceof Class
        assert Symbol instanceof Class
        assert Null instanceof Class
        assert String instanceof Class
        assert Bytes instanceof Class
        assert Tuple instanceof Class
        assert List instanceof Class
        assert Instance instanceof Class
        assert Class instanceof Class
        assert Shape instanceof Class
        assert Function instanceof Class
        assert BuiltinFunction instanceof Class
        assert Fiber instanceof Class
        assert Future instanceof Class
        assert Exception instanceof Class
        assert ImportException instanceof Class
        assert AssertionException instanceof Class
    }

    static func test_builtin_functions {
        assert write instanceof Function
        assert print instanceof Function
        assert prompt instanceof Function
        assert exit instanceof Function
        assert stopwatch instanceof Function
    }
}





