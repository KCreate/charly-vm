/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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

import "unittest"

const result = unittest(->(describe, it, assert, context) {

  const testcases = [

    // Interpreter specs
    ["Arithmetic operations",       "./interpreter/arithmetic.ch"],
    ["Bitwise operations",          "./interpreter/bitwise.ch"],
    ["Classes",                     "./interpreter/classes.ch"],
    ["Comments",                    "./interpreter/comments.ch"],
    ["Comparisons",                 "./interpreter/comparisons.ch"],
    ["Exceptions",                  "./interpreter/exceptions.ch"],
    ["External Files",              "./interpreter/external-files.ch"],
    ["Functions",                   "./interpreter/functions.ch"],
    ["Guard",                       "./interpreter/guard.ch"],
    ["Identifiers",                 "./interpreter/identifiers.ch"],
    ["Loops",                       "./interpreter/loops.ch"],
    ["Objects",                     "./interpreter/objects.ch"],
    ["Primitives",                  "./interpreter/primitives.ch"],
    ["Switch",                      "./interpreter/switch.ch"],
    ["Ternary",                     "./interpreter/ternary.ch"],
    ["Unless",                      "./interpreter/unless.ch"],

    // Standard library specs
    ["Heaps",                       "./stdlib/heaps.ch"]
  ]

  // Loads and runs all the test cases sequentially
  // TODO: schedule them asynchronously maybe???
  testcases.each(->(test) {
    const module = import test[1]
    describe(test[1], ->{
      module(describe, it, assert, context)
    })
  })

})

unittest.display_result(result)
