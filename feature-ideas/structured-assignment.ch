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

// The following transformations should be transformations on the AST only.
// The result of a structured assignment cannot be used as another expression
// e.g. the following code would be invalid:
//
//  const obj = { name: "leonard", age: 20 }
//  const a = <exp> + ({name, age} = obj)
//                                 ^
//                                 |
//                                 +- structured object assignment cannot
//                                    be used as expression!

// Object destructuring initialisation
//
// Desugared to:
//
// let num
// let foo
// %tmp = obj
// num = %tmp.num
// foo = %tmp.foo
let {num, foo} = obj

// Object destructuring assignment
//
// Desugared to:
//
// %tmp = obj
// num = %tmp.num
// foo = %tmp.foo
{num, foo} = obj

// Object destructuring self assignment
//
// Desugared to:
//
// %tmp = obj
// other.num = %tmp.num
// other.foo = %tmp.foo
{other.num, other.foo} = obj

// error! cannot use member assignment as variable name
let {other.num, other.foo} = obj

// %tmp = obj
// self.num = %tmp.num
// self.foo = %tmp.foo
{@num, @foo} = obj

// %tmp = obj
// foo.bar.baz = %tmp.baz
// foo.bar.qux = %tmp.qux
{foo.bar.baz, foo.bar.qux} = obj

// %tmp = obj
// foo[0].name = %tmp.name
// foo[0].age = %tmp.age
{foo[0].name, foo[0].age} = obj

// %tmp = obj
// obj = %tmp.obj
{ obj } = obj

// %tmp = get_person()
// name = %tmp.name
// age = %tmp.age
// height = %tmp.height
{name, age, height} = get_person()

// error! one identifier minimum
let {} = obj
{} = obj

// error! expected structured assignment target, not object literal
{ name: "leonard", age: 20 } = obj
