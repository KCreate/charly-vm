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

// Default argument refactor:
//    Default arguments no longer need to be implemented as arguments
//    array length checks. Because we know how many arguments are passed
//    in, and each function knows at compile-time how many default arguments
//    it has, we can simply jump to a specific offset of the function and
//    let it generate all default arguments in sequence.
//
//    The function prologue gets implemented as a series of branch
//    instructions which jump to the relevant default argument initializer.
//    Default argument intializers are compiled in definition order, allowing
//    later arguments to reference previous ones and to avoid any additional
//    jumps, letting the code just flow into the main body.
//
//    The function 'func foo(a = 1, b = 2) { a + b }' would get compiled to the following
//    bytecodes
//
//    foo {
//      .LARG0    // if given 0 arguments
//        a = 1
//      .LARG1    // if given 1 argument
//        b = 2
//      .MAIN     // if given 2 or more arguments (depending on argc of function)
//        readlocal 0, 0
//        readlocal 0, 1
//        add
//        return
//    }
//
//    Calls to the function with enough arguments simply jump directly to the
//    MAIN block and skip all default argument initialisation code.
//
//
// Argument spread:
//    The argument spread construct can be implemented as an additional flag
//    in the Function struct. It stores wether the last argument of the function
//    should be an array containing the last and all additional arguments passed
//    to the function.
//
//    The packing of the arguments into the array is performed inside the
//    call_function method of the VM. If no arguments are applicable, it simply
//    pushes an empty array.
