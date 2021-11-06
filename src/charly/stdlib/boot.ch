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

func determine_max_stack_size {
    let counter = 0

    try {
        func recurse {
            counter += 1
            recurse()
        }
        recurse()
    } catch(e) {
        return counter
    }

    return 100
}

func foreach_until(tuple, cb, sentinel = null) {
    let i = 0
    loop {
        const value = tuple[i]
        if value == sentinel {
            break
        }

        cb(value, i, tuple)
        i += 1
    }
}

let filename = ARGV[0]

switch (filename) {
    case "repl" {
        foreach_until(ARGV, ->(a, i) {
            ARGV[i] = "#{i}: {a}"
        })

        return "repl: {ARGV}"
    }

    default {
        const stack_size = determine_max_stack_size()
        return ("stack height limit: {stack_size} frames", ARGV)
    }
}