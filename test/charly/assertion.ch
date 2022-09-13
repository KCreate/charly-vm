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

export class AssertionTest {
    static func test_assert {
        assert assert_throws(->{
            let a = false
            assert a
        }).message == "Failed assertion"

        assert assert_throws(->{
            let a = 0
            assert a
        }).message == "Failed assertion"

        assert assert_throws(->{
            let a = null
            assert a
        }).message == "Failed assertion"
    }

    static func test_assert_comparison {
        assert assert_throws(->{
            assert 100 == 200
        }).message == "Failed assertion (100 == 200)"

        assert assert_throws(->{
            assert 100 > 150
        }).message == "Failed assertion (100 > 150)"

        assert assert_throws(->{
            assert 100 != 100
        }).message == "Failed assertion (100 != 100)"
    }

    static func test_assert_message {
        assert assert_throws(->{
            assert false : "some error"
        }).message == "some error"
    }

    static func test_assert_comparison_message {
        assert assert_throws(->{
            assert 100 == 200 : "some error"
        }).message == "some error (100 == 200)"

        assert assert_throws(->{
            assert 100 > 150 : "some error"
        }).message == "some error (100 > 150)"

        assert assert_throws(->{
            assert 100 != 100 : "some error"
        }).message == "some error (100 != 100)"
    }

    static func test_assert_exception_components {
        const (value) = assert_throws(->assert null).components
        assert value == null
    }

    static func test_assert_comparison_exception_components {
        const (left, op, right) = assert_throws(->assert 100 == 200).components
        assert left == 100
        assert op == "=="
        assert right == 200
    }
}












