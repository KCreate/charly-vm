/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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

//Charly.set_default_exception_handler(->(e) {
  //print("Caught an exception: " + e)
//})

//try {
  //defer(->{
    //throw "hello world"
  //})
//} catch(e) {
  //print(e)
//}

const buf = String.Buffer(2)

print(buf)
buf.write("hello world")

print(buf)
buf.write("hello world")

print(buf)
buf.write("hello world")

print(buf)
buf.write("hello world")

buf.reserve(256)

print(buf)
buf.write("hello world")

buf.reserve(128)

print(buf)
buf.write("hello world")

buf.reserve(4)

print(buf)
buf.write("hello world")

print(buf)
buf.write("hello world")

const result = buf.str()
print(result)
