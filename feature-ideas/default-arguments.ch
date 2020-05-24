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

// Implementation:  Default arguments need to be implemented inside
//                  the code-generation layer of the compiler. The
//                  VM itself doesn't actually have to know about this
//                  whole thing.
//
//                  A check inside the function argument parsing code
//                  looks out for '=' signs, skips them and parses an expression.
//                  It also sets a flag, to let subsequent passes know that
//                  the remaining arguments need to have default arguments too.
//
//                  The function node gets an additional parameter which
//                  denotes the minimum required argument count. This
//                  is the value which gets used by the VM to check if enough
//                  arguments have been passed to the function.

// Default arguments can be set by appending an identifier with an
// '=' sign and an expression. The expression is executed as soon
// as the function is getting called. It is effectively compiled to
// the following pseudo-code:
//
//    // Original code
//    func f(a, b = 1, c = 2) = a + b + c
//
//    // De-sugared pseudo-code
//    func f(a, b, c) {
//      if arguments.length < 1 b = 1
//      if arguments.length < 2 c = 2
//
//      // <block>
//    }
func f(a, b, c = 1) = (a + b) * c

// After a default value has been defined,
// all following arguments need default values too
//
//               +- Missing default argument for 'c'
//               |
//               v
func g(a, b = 1, c) = a + b + c
