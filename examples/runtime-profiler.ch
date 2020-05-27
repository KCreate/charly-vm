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

// The purpose of this file is to touch every instruction supported by the machine at least once
// It is used as the sample file when compiling the production binary using the
// profile guided optimization flags for clang
//
// Usage:
// make profiledproduction

let i = 0
let iterations = 100000

const thrown_exceptions = []
while (i < iterations) {
  let a = 10
  let b = 10
  let c = a + b

  let container = {
    nums: [1, 2, 3, 4],
    obj: {
      num: 0,
      method: func {
        @num += 1
      }
    }
  }

  container.nums << 5
  container.nums << 6

  container.obj.method()
  container.obj.method()
  container.obj.method()

  if (container.obj.num == 3) {
    container.obj.method()
  }

  const k1 = "obj"
  const k2 = "num"
  if (container[k1][k2] == 3) {
    container.obj.method()
  }

  unless (container[k1][k2] == 3) {
    container.obj.method()
  }

  const someobj = { num: 0 }
  const k3 = "num"
  ->{}(someobj[k3] = 100) // causes a setmembervaluepush instruction
  someobj[k3] = 100 // causes a setmembervalue instruction
  someobj[k3] += 1
  someobj[k3] += 1
  someobj[k3] += 1

  const arguments_copy = [$0, $1, $2, $3, $4, $5, $6]

  let someclass_repetitions = 0
  const someclass = class SomeClass extends class Base {
    property boye

    method {
      someclass_repetitions += 1
    }
  } {
    call {
      @method()
      @method()
      @method()
    }
  }

  const myobj = new someclass(25)
  myobj.call()
  myobj.call()
  myobj.call()

  let correct = someclass_repetitions == 9

  func somefunc {}
  somefunc()
  somefunc()
  somefunc()
  somefunc()

  try {
    throw {
      message: "hello world"
    }
  } catch (e) {
    thrown_exceptions << e.message
  }

  try {} catch (e) {}

  const types = [typeof [1, 2], typeof 25, typeof "hello world"]

  const n1 = 25
  const n2 = 5
  const a1 = [1, 2, 3]
  const a2 = [1, 2, 3]
  const s1 = "hello world"
  const s2 = "hello world 2"
  const calculations = [
    n1 + n2,
    n1 - n2,
    n1 * n2,
    n1 / n2,
    n1 % n2,
    n1 ** n2,
    n1 == n2,
    n1 ! n2,
    n1 < n2,
    n1 > n2,
    n1 >= n2,
    n1 <= n2,
    n1 << n2,
    n1 >> n2,
    n1 & n2,
    n1 | n2,
    n1 ^ n2,
    +n1,
    -n1,
    !n1,
    ~n1,

    a1 + a2,

    s1 == s2,
    s1 < s2,
    s1 > s1,
    s1 <= s2,
    s1 >= s2,
    s1 ! s2,

    s1 + s2,
    s1 * 0,
    s1 * 1,
    s1 * 2,
    s1 * 10
  ]

  const floatnums = [
    1.1,
    2.2,
    3.3,
    4.4,
    5.5,
    6.6,
    7.7,
    8.8,
    9.9
  ]

  func readarrayindex {
    const nums = [$0, $1, $2, $3, $4, $5]
  }

  readarrayindex()

  // Check long and short string optimizations
  const strings = [
    "abc",
    "abcdef",
    "abcdefabcdef",
    "abcdefabcdefabcdefabcdef",
    "abcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdef",
    "abcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdef" * 10
  ]

  func generatorcounter(a) {
    let i = 0
    while i < a - 1 {
      yield i
      i += 1
    }
    a
  }

  const counter = generatorcounter(10)
  let sum = 0
  while counter {
    sum += counter()
  }

  i += 1

  // branchlt, branchgt, branchle instructions
  let check
  if 100 >= 100 check = true
  if 100 <= 100 check = true
  if 100 > 100 check = true
  if 100 ! 100 check = true
}
