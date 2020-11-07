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

export = ->(describe, it, assert) {

  it("adds properties to objects", ->{
    class Box {}
    const myBox = new Box()
    myBox.name = "charly"
    myBox.age = 16

    assert(myBox.name, "charly")
    assert(myBox.age, 16)
  })

  it("copies an object", ->{
    const a = { a: 1, b: 2 }
    const b = a.copy()

    assert(b.a, 1)
    assert(b.b, 2)

    a.a = 100
    a.b = 200

    assert(b.a, 1)
    assert(b.b, 2)
  })

  it("correctly encodes symbols", ->{
    const obj = {}

    obj["foo"]        = "foo"
    obj[25]           = "25"
    obj[true]         = "true"
    obj[false]        = "false"
    obj[null]         = "null"
    obj[NaN]          = "NaN"
    obj[25]           = "25"
    obj[-200]         = "-200"
    obj[[1, 2]]       = "<array>"
    obj[{ val: 2 }]   = "<object>"
    obj[class Foo {}] = "<class>"

    assert(Object.keys(obj).contains("foo"))
    assert(Object.keys(obj).contains("25"))
    assert(Object.keys(obj).contains("true"))
    assert(Object.keys(obj).contains("false"))
    assert(Object.keys(obj).contains("null"))
    assert(Object.keys(obj).contains("nan"))
    assert(Object.keys(obj).contains("25"))
    assert(Object.keys(obj).contains("-200"))
    assert(Object.keys(obj).contains("<array>"))
    assert(Object.keys(obj).contains("<object>"))
    assert(Object.keys(obj).contains("<class>"))
  })

  it("adds functions to objects", ->{
    class Box {}
    const myBox = new Box()
    myBox.name = "charly"
    myBox.age = 16
    myBox.to_s = func() {
      assert(self == myBox, true)
      myBox.do_stuff = func() {
        "it works!"
      }

      myBox.name + " - " + myBox.age
    }

    assert(myBox.name, "charly")
    assert(myBox.age, 16)
    assert(myBox.do_stuff, null)
    assert(myBox.to_s(), "charly - 16")
    assert(myBox.do_stuff(), "it works!")
  })

  it("assigns via index expressions", ->{
    const box = {
      name: "test"
    }

    box["name"] = "it works"

    assert(box["name"], "it works")
  })

  describe("pretty-printing", ->{

    it("renders arrays", ->{
      let arr = [1, 2, 3, 4]

      let render = arr.to_s()

      assert(render, "[1, 2, 3, 4]")
    })

    it("prints circular arrays", ->{
      let a = []
      let b = []

      a << b
      b << a

      let render = a.to_s()

      assert(render, "[[<...>]]")
    })

    it("prints circular objects", ->{
      const a = {}
      a.a = a

      const render = a.to_s()
      assert(render, "{\n  a = <...>\n}")
    })

  })

  describe("stdlib methods", ->{

    describe("is_a", ->{
      class A {}
      class B extends A {}
      class C extends B {}
      class D extends C {}

      assert((new D()).is_a(D))
      assert((new D()).is_a(C))
      assert((new D()).is_a(B))
      assert((new D()).is_a(A))
      assert((new D()).is_a(Object))

      assert((new D()).is_a(Value))
      assert((new C()).is_a(D), false)
      assert((new C()).is_a(Number), false)
      assert((new C()).is_a(String), false)
      assert("hello".is_a(String), true)

      assert([1, 2].is_a(Array))
      assert(true.is_a(Boolean))
      assert((class {}).is_a(Class))
      assert((->{}).is_a(Function))
      assert(null.is_a(Null))
      assert(25.is_a(Number))
      assert({ a: 25 }.is_a(Object))

      assert([1, 2].is_a(Value))
      assert(true.is_a(Value))
      assert((class {}).is_a(Value))
      assert((->{}).is_a(Value))
      assert(null.is_a(Value))
      assert(25.is_a(Value))
      assert({ a: 25 }.is_a(Value))
    })

    describe("tap", ->{
      "leonard".tap(->(name) {
        assert(name, "leonard")
      })

    })

    describe("pipe", ->{
      let v = 0

      const a = ->v += $0
      const b = ->v *= $0
      const c = ->v -= $0

      10.pipe(a, b, c)

      assert(v, 90)
    })

    describe("transform", ->{
      const r = 10.transform(
        ->(v) {
          v + 10
        },
        ->(v) {
          v * 10
        },
        ->(v) {
          v / 2
        }
      )

      assert(r, 100)
    })

    describe("delete_key", ->{
      const obj = {}

      obj.foo = 25
      obj.bar = 50
      obj.baz = 75

      assert(obj.foo, 25)
      assert(obj.bar, 50)
      assert(obj.baz, 75)
      assert(Object.keys(obj).length, 3)

      Object.delete(obj, "foo")
      Object.delete(obj, "bar")
      Object.delete(obj, "baz")

      assert(obj.foo, null)
      assert(obj.bar, null)
      assert(obj.baz, null)
      assert(Object.keys(obj).length, 0)
    })

  })

}
