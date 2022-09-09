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

export class TruthynessTest {
    static func test_truthyness {
        assert !!null == false
        assert !!false == false
        assert !!NaN == false
        assert !!0 == false
        assert !!0.0 == false

        assert !!true == true
        assert !!100 == true
        assert !!192.168 == true
        assert !!"" == true
        assert !!"hi" == true
        assert !!"hello world this is a test" == true
        assert !!() == true
        assert !!(100, 200) == true
        assert !![100, 200] == true
        assert !!(func foo {}) == true
        assert !!(->{}) == true
        assert !!(class Foo {}) == true
        assert !!(class Foo {})() == true
        assert !!(builtin_exit) == true
        assert !!(Exception("test exception")) == true
    }
}





