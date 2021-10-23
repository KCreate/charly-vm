/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2021 Leonard Schütz
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

func determine_max_stack_size {
    let counter = 0
    try {
        func recurse {
            counter += 1
            recurse()
        }
        recurse()
    } catch(e) {
        return (e, counter)
    }
}

func times(n, cb) {
    let i = 0
    loop {
        if i == n break
        cb(i);
        i += 1
    }
}

func repeat_string(string, n) {
    let result = ""

    times(n, ->{
        result = "{result}{string}"
    })

    return result
}

return (
    determine_max_stack_size(),
    (0, repeat_string("x", 0)),
    (1, repeat_string("x", 1)),
    (5, repeat_string("x", 5)),
    (6, repeat_string("x", 6)),
    (7, repeat_string("x", 7)),
    (8, repeat_string("x", 8)),
    (9, repeat_string("x", 9)),
    (10, repeat_string("x", 10)),
    (100, repeat_string("x", 100)),
    (1000, repeat_string("x", 1000)),
    (3000, repeat_string("x", 3000))
)