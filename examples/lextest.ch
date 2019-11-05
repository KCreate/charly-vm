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

"hello world".each(->(c) {
  print(c)
})

const r0 = "hello world".map(->(c) {
  "_" + c + "_"
})
print(r0)

const r1 = "hello world".filter(->(c) {
  if c == " " return false
  true
})
print(r1)

const r2 = "      hello".ltrim()
print(r2)

const r3 = "hello      ".rtrim()
print(r3)

const r4 = "   hello   ".trim()
print(r4)

const r5 = "abcdefg".reverse()
print(r5)

const r6 = "hello|world|whats|up|today!|ääööüü".split("|")
print(r6)

const r7 = "hello world".index("world")
print(r7)

const r8 = "goodbye world world".index("world", 2)
print(r8)

const r9 = "hello world".rindex("world")
print(r7)

const r10 = "goodbye world world".rindex("world")
print(r10)

const r11 = "goodbye world world".rindex("world", -2)
print(r11)

const r12 = "hello world how are you äöü Ç ±±#œ∑€®†Ω°ª@ƒ∂".split("")
print(r12)
