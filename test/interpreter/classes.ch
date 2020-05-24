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

  it("creates a new class", ->{
    class Person {
      property name
      property age
    }

    const p1 = new Person("Felix", 21)
    const p2 = new Person("Marcus", 28)

    assert(p1.name, "Felix")
    assert(p1.age, 21)
    assert(p2.name, "Marcus")
    assert(p2.age, 28)
  })

  it("generates default constructors for classes", ->{
    class Box {
      property foo
      property bar
      property baz
    }

    const b1 = new Box(1, 2, 3)
    assert(b1.foo, 1)
    assert(b1.bar, 2)
    assert(b1.baz, 3)

    try {
      const b2 = new Box()
      assert(b2.foo, null)
      assert(b2.bar, null)
      assert(b2.baz, null)
    } catch(e) {
      print(e)
      print("got an error")
    }
  })

  it("adds methods to classes", ->{
    class Box {
      property value

      func set(@value) {}
    }

    const myBox = new Box(0)
    assert(myBox.value, 0)
    myBox.set("this works")
    assert(myBox.value, "this works")
  })

  it("calls methods from parent classes", ->{
    class Box {
      func foo() {
        "it works"
      }
    }

    class SpecialBox extends Box {}

    const myBox = new SpecialBox()
    assert(myBox.foo(), "it works")
  })

  it("calls methods from parent classes inside child methods", ->{
    class Box {
      func foo() {
        "it works"
      }
    }

    class SpecialBox extends Box {
      func bar() {
        @foo()
      }
    }

    const myBox = new SpecialBox()
    assert(myBox.bar(), "it works")
  })

  it("sets props on classes", ->{
    class A {}

    assert(A.name, "A")
  })

  it("sets props on child classes", ->{
    class A {}
    class B extends A {}

    assert(B.name, "B")
    assert(B.parent_class.name, "A")
  })

  it("creates static properties on classes", ->{
    class Box {
      static property count = 0

      func constructor() {
        Box.count += 1
      }
    }

    new Box()
    new Box()
    new Box()
    new Box()

    assert(Box.count, 4)
  })

  it("creates static methods on classes", ->{
    class Box {
      static func do_something() {
        "static do_something"
      }

      func do_something() {
        "instance do_something"
      }
    }

    const myBox = new Box()
    assert(myBox.do_something(), "instance do_something")
    assert(Box.do_something(), "static do_something")
  })

  it("passes the class via the self identifier on static methods", ->{
    class Box {
      static func foo() {
        assert(self, Box)
      }
    }

    Box.foo()
  })

  it("inserts quick access identifiers into constructor calls", ->{
    class Box {
      property value

      func constructor {
        @value = $0 + $1 + $2
      }
    }

    let box = new Box(1, 2, 3)
    assert(box.value, 6)
  })

  it("set the __class property", ->{
    class Box {}
    const myBox = new Box()

    assert(myBox.klass, Box)
  })

  it("inherits methods from parent classes", ->{
    class Foo {
      func a() { "method a" }
    }

    class Bar extends Foo {
      func b() { "method b" }
    }

    class Baz extends Bar {}

    let mybaz = new Baz()
    assert(mybaz.a(), "method a")
    assert(mybaz.b(), "method b")
  })

  it("calls parent class constructors", ->{
    class A {
      property v1

      func constructor(@v1) {}
    }

    class B extends A {
      property v2

      func constructor(v1, @v2) {}
    }

    let a = new B(10, 20)
    assert(a.v1, 10)
    assert(a.v2, 20)
  })

  it("handles exceptions inside constructors correctly", ->{
    class A {
      func constructor {
        throw "test"
      }
    }

    let caught_exception = null
    try {
      new A()
    } catch(e) {
      caught_exception = e
    }

    assert(typeof caught_exception, "string")
    assert(caught_exception, "test")
  })

  it("throws an exception when trying to call a class", ->{
    class A {}

    let caught_exception
    try {
      A()
    } catch(e) {
      caught_exception = e
    }

    assert(typeof caught_exception, "object")
    assert(caught_exception.message, "Attempted to call a non-callable type: class")
  })

  it("changes the name of a class", ->{
    class A {}
    assert(A.name, "A")
    A.name = "B"
    assert(A.name, "B")
  })

  it("changes the parent_class of a class", ->{
    class A {}
    class B {}
    class C extends A {}

    assert(C.parent_class, A)
    C.parent_class = B
    assert(C.parent_class, B)
  })

  describe("correctly unpacks call nodes", ->{

    it("parses the member call syntax", ->{
      const c = {
        c: class Foo {}
      }

      const a = new c.c()
      assert(a.klass.name, "Foo")
    })

    it("parses the index call syntax", ->{
      const c = [class Foo {}]

      const a = new c[0]()
      assert(a.klass.name, "Foo")
    })

  })

}
