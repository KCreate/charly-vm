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

// Implementation:  The runtime Function struct gets a new flag which
//                  stores if the function's last argument has a spread-operator
//                  If yes, the VM will pack all the correct arguments into
//                  an array and pass that as the last parameter.
//
//                  This needs to be implemented inside the VM::call_function
//                  method and should not require any changes to the code generation
//                  facilities.
//
//                  The previous 'arguments' identifier gets removed
//                  This means the instructions readarrayindex and setarrayindex can
//                  also be removed

// The '...' spread operator can be used to access all remaining arguments
func h(a, b, c...) = _

// The argument followed by the spread operator has to be
// the last argument of the function
//
//                 +- Can't have an argument after
//                 |  a argument spread operator
//                 |
//                 v
func j(a, b, c..., d) = _

// The following function declaration effectively renames the
// 'argument' identifier.
func k(a...) = _

// Arguments before can have default arguments
//
// l()            => [100, []]
// l(1)           => [1, []]
// l(1, 2)        => [1, [2]]
// l(1, 2, 3)     => [1, [2, 3]]
func l(num = 100, other...) = [num, other]
