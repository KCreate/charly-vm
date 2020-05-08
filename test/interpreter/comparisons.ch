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

export = ->(describe, it, assert) {

  it("compares integers", ->{
    assert(1 == 1, true)
    assert(1 ! -1, true)
    assert(200 == 200, true)
    assert(-200 == -200, true)
    assert(-500 ! 500, true)
    assert(0 == 0, true)

    assert(1 == 2, false)
    assert(1 == -1, false)
    assert(200 == 300, false)
    assert(-200 == -201, false)
    assert(-500 == 500, false)
  })

  it("compares floats", ->{
    assert(3.5 == 3.5, true)
    assert(3.1111 == 3.1111, true)
    assert(-25.5 == -25.5, true)
    assert(0.00 == 0.00, true)
    assert(0.0000001 == 0.0000001, true)

    assert(3.5 == 3.4, false)
    assert(3.1111 == 3.1112, false)
    assert(-25.5 == 25.5, false)
    assert(0.00 == 0.01, false)
    assert(0.0000001 == 0.0000002, false)
    assert(9.666 == 9, false)

    assert(NaN == NaN, true)
    assert(NaN == 0, false)
    assert(0 == NaN, false)
  })

  it("compares booleans", ->{
    assert(false == false, true)
    assert(false == true, false)
    assert(true == false, false)
    assert(true == true, true)

    assert(0 == false, false)
    assert(1 == true, false)
    assert(-1 == true, false)
    assert("" == true, false)
    assert("test" == true, false)
    assert([] == true, false)
    assert([1, 2] == true, false)
    assert(null == false, false)
    assert({ name: "charly" } == true, false)
    assert(func() {} == true, false)
    assert(class Test {} == true, false)
  })

  it("compares strings", ->{
    assert("test" == "test", true)
    assert("" == "", true)
    assert("leeöäüp" == "leeöäüp", true)
    assert("2002" == "2002", true)
    assert("asdlkasd" == "asdlkasd", true)
  })

  it("compares objects", ->{
    assert({} == {}, false)
    assert({ a: 1 } == {}, false)
    assert({ a: 1 } == { a: 1 }, false)

    const me = {
      name: "charly"
    }

    assert(me == me, true)
    assert(me.name == me.name, true)
  })

  it("compares functions", ->{
    assert(func() {} == func() {}, false)
    assert(func(arg) {} == func(arg) {}, false)
    assert(func(arg) { arg + 1 } == func(arg) { arg + 1 }, false)
    assert(func() { 2 } == func(){ 2 }, false)

    func add(a, b) = a + b
    func add2(a, b) = a + b

    assert(add == add, true)
    assert(add == add2, false)
  })

  it("compares arrays", ->{
    assert([1, 2, 3] == [1, 2, 3], true)
    assert([] == [], true)
    assert([false] == [false], true)
    assert([[1, 2], "test"] == [[1, 2], "test"], true)

    assert([1] == [1, 2], false)
    assert(["", "a"] == ["a"], false)
    assert([1, 2, 3] == [1, [2], 3], false)
    assert([""] == [""], true)
  })

  it("returns false if two values are not equal", ->{
    assert(2 == 4, false)
    assert(10 == 20, false)
    assert(-20 == 20, false)
    assert(2 == 20, false)

    assert(2 ! 4, true)
    assert(10 ! 20, true)
    assert(-20 ! 20, true)
    assert(2 ! 20, true)
  })

  describe("> operator", ->{
    assert(2 > 5, false)
    assert(10 > 10, false)
    assert(20 > -20, true)
    assert(4 > 3, true)
    assert(0 > -1, true)

    assert("test" > "test", false)
    assert("whatsup" > "whatsu", true)
    assert("test" > 2, false)
    assert("test" > "tes", true)
    assert(2 > "asdadasd", false)
    assert("" > "", false)
    assert(false > true, false)
    assert(25000 > false, false)
    assert(0.222 > "0.222", false)
    assert(null > "lol", false)
  })

  describe("< operator", ->{
    assert(2 < 5, true)
    assert(10 < 10, false)
    assert(20 < -20, false)
    assert(4 < 3, false)
    assert(0 < -1, false)

    assert("test" < "test", false)
    assert("whatsup" < "whatsu", false)
    assert("test" < 2, false)
    assert("test" < "tes", false)
    assert(2 < "asdadasd", false)
    assert("" < "", false)
    assert(false < true, false)
    assert(25000 < false, false)
    assert(0.222 < "0.222", false)
    assert(null < "lol", false)
  })

  describe(">= operator", ->{
    assert(5 >= 2, true)
    assert(10 >= 10, true)
    assert(20 >= 20, true)
    assert(4 >= 3, true)
    assert(0 >= -1, true)

    assert("test" >= "test", true)
    assert("whaaatsup" >= "whatsup", true)
    assert("lol" >= "lol", true)
    assert("abc" >= "def", true)
    assert("small" >= "reaaalllybiiig", false)
  })

  describe("<= operator", ->{
    assert(2 <= 5, true)
    assert(10 <= 10, true)
    assert(20 <= -20, false)
    assert(4 <= 3, false)
    assert(200 <= 200, true)

    assert("test" <= "test", true)
    assert("whaaatsup" <= "whatsup", false)
    assert("lol" <= "lol", true)
    assert("abc" <= "def", true)
    assert("small" <= "reaaalllybiiig", true)
  })

  describe("not operator", ->{
    assert(!false, true)
    assert(!true, false)
    assert(!0, true)
    assert(!25, false)
    assert(!"test", false)
  })

  describe("AND comparison", ->{
    assert(true && true, true)
    assert(true && false, false)
    assert(false && true, false)
    assert(false && false, false)
  })

  describe("OR comparison", ->{
    assert(true || true, true)
    assert(true || false, true)
    assert(false || true, true)
    assert(false || false, false)
  })

  describe("conditional assignment", ->{
    let a = 25
    let b = null
    let c = false

    let d

    d = a || b
    assert(typeof d, "number")

    d = b || c
    assert(typeof d, "boolean")

    d = b || a
    assert(typeof d, "number")

    d = c || b
    assert(typeof d, "null")
  })

  describe("null values", ->{
    assert(25 ! null, true)
    assert(null ! null, false)
    assert(null == null, true)
    assert(null == false, false)
    assert(null == true, false)
    assert(null ! false, true)
    assert(null ! true, true)
  })

  describe("comparison operator overloading", ->{

    class Box {
      property value

      func constructor(@value) = null
      func @"=="(other) = @value == other.value
    }

    const a = Box(100)
    const b = Box(200)
    const c = Box(300)

    assert(a == b, false)
    assert(b == c, false)
    assert(a == c, false)
    assert(a == a, true)
    assert(b == b, true)
    assert(c == c, true)
  })
}
