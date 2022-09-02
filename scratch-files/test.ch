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

func test_tuple_spread {
    const a = (1, 2, 3, 4)
    const b = "abcäää"
    const c = [5, 6, 7, 8]
    const t = (true, ...a, false, ...b, null, ...c, 2000)

    assert t == (true, 1, 2, 3, 4, false, "a", "b", "c", "ä", "ä", "ä", null, 5, 6, 7, 8, 2000)
}
test_tuple_spread()

func test_empty_tuple_spread {
    const a = ()
    const b = ""
    const c = []
    const t = (...a, ...b, ...c)
    assert t == ()
}
test_empty_tuple_spread()

func test_list_spread {
    const a = (1, 2, 3, 4)
    const b = "abcäää"
    const c = [5, 6, 7, 8]
    const t = [true, ...a, false, ...b, null, ...c, 2000]

    assert t == [true, 1, 2, 3, 4, false, "a", "b", "c", "ä", "ä", "ä", null, 5, 6, 7, 8, 2000]
}
test_list_spread()

func test_empty_list_spread {
    const a = ()
    const b = ""
    const c = []
    const t = [...a, ...b, ...c]
    assert t == []
}
test_empty_list_spread()

func test_function_argument_spread {
    func foo(...args) = args
    assert foo(...(1, 2, 3, 4)) == (1, 2, 3, 4)
    assert foo(...[1, 2, 3, 4]) == (1, 2, 3, 4)
    assert foo(..."hellö") == ("h", "e", "l", "l", "ö")
}
test_function_argument_spread()

func test_unpacksequence_assignment {
    const (f, g, h, i) = (1, 2, 3, 4)
    assert (f, g, h, i) == (1, 2, 3, 4)

    const (j, k, l, m) = [1, 2, 3, 4]
    assert (j, k, l, m) == (1, 2, 3, 4)
}

test_unpacksequence_assignment()

func test_unpacksequencespread_assignment {
    const (c, d, ...rest2) = (1, 2, 3, 4)
    assert (c, d, rest2) == (1, 2, (3, 4))

    const (e, f, ...rest3) = [1, 2, 3, 4]
    assert (e, f, rest3) == (1, 2, (3, 4))
}

test_unpacksequencespread_assignment()

