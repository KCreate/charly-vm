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

    const b2 = new Box()
    assert(b2.foo, null)
    assert(b2.bar, null)
    assert(b2.baz, null)

    const b3 = new Box(1, 2)
    assert(b3.foo, 1)
    assert(b3.bar, 2)
    assert(b3.baz, null)
  })

  it("adds methods to classes", ->{
    class Box {
      property value

      set(@value) {}
    }

    const myBox = new Box(0)
    assert(myBox.value, 0)
    myBox.set("this works")
    assert(myBox.value, "this works")
  })

  it("calls methods from parent classes", ->{
    class Box {
      foo() {
        "it works"
      }
    }

    class SpecialBox extends Box {}

    const myBox = new SpecialBox()
    assert(myBox.foo(), "it works")
  })

  it("calls methods from parent classes inside child methods", ->{
    class Box {
      foo() {
        "it works"
      }
    }

    class SpecialBox extends Box {
      bar() {
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
    assert(B.parent.name, "A")
  })

  it("creates static properties on classes", ->{
    class Box {
      static property count = 0

      constructor() {
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
      static do_something() {
        "static do_something"
      }

      do_something() {
        "instance do_something"
      }
    }

    const myBox = new Box()
    assert(myBox.do_something(), "instance do_something")
    assert(Box.do_something(), "static do_something")
  })

  it("passes the class via the self identifier on static methods", ->{
    class Box {
      static foo() {
        assert(self, Box)
      }
    }

    Box.foo()
  })

  it("inserts quick access identifiers into constructor calls", ->{
    class Box {
      property value

      constructor {
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
      a() { "method a" }
    }

    class Bar extends Foo {
      b() { "method b" }
    }

    class Baz extends Bar {}

    let mybaz = new Baz()
    assert(mybaz.a(), "method a")
    assert(mybaz.b(), "method b")
  })

  it("calls parent class constructors", ->{
    class A {
      property v1

      constructor(@v1) {}
    }

    class B extends A {
      property v2

      constructor(v1, @v2) = super(v1)
    }

    let a = new B(10, 20)
    assert(a.v1, 10)
    assert(a.v2, 20)
  })

  it("handles exceptions inside constructors correctly", ->{
    class A {
      constructor {
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

  it("allows the func keyword to be ommitted in class declarations", ->{
    class Person {
      property name
      property age

      constructor(name, age) {
        @name = "test" + name
        @age = age + 5
      }

      do_stuff {
        return [@name, @age]
      }

      static do_more_stuff {
        return "this is a static function"
      }
    }

    const p = new Person("leonard", 20)
    assert(p.do_stuff(), ["testleonard", 25])
    assert(Person.do_more_stuff(), "this is a static function")
  })

  it("allows blocks to be omitted", ->{
    class Color {
      set_r(@r)
      property r

      property g
      property b

      set_g(@g)
      set_b(@b)
    }

    const c = new Color(1, 2, 3)
    assert(c.r, 1)
    assert(c.g, 2)
    assert(c.b, 3)

    c.set_r(20)
    c.set_g(30)
    c.set_b(40)

    assert(c.r, 20)
    assert(c.g, 30)
    assert(c.b, 40)
  })

  it("throws an exception when the parent_class isn't a class", ->{
    const cases = [
      ->class A extends null {},
      ->class A extends 25 {},
      ->class A extends "test" {},
      ->class A extends [1, 2] {},
      ->class A extends {a: 1} {}
    ]

    const expected_error_message = "Can't extend from non class value"

    cases.each(->(c) {
      assert.exception(c, ->(e) assert(e.message, expected_error_message))
    })
  })

  it("correctly instantiates subclasses with no constructor", ->{
    class A {
      property name
      property age
    }

    class B extends A {}

    const obj = new B("foo", 25)

    assert(obj.name, "foo")
    assert(obj.age, 25)
  })

  it("calls super constructor", ->{
    let a_called = false
    let c_called = false
    let d_called = false

    class A {
      constructor {
        a_called = true
      }
    }

    class B extends A {}

    class C extends B {
      constructor {
        super()
        c_called = true
      }
    }

    class D extends C {
      constructor {
        super()
        d_called = true
      }
    }

    const obj = new D()

    assert(a_called)
    assert(c_called)
    assert(d_called)
  })

  it("calls super member methods", ->{
    class A {
      foo = "a foo"
      bar = "a bar"
    }

    let b_called = false
    let testval = null

    class B extends A {
      constructor {
        b_called = true
        testval = super.bar()
      }

      foo = super()
      bar = super.bar()
      baz = super.bar()
    }

    const obj = new B()

    assert(obj.foo(), "a foo")
    assert(obj.bar(), "a bar")
    assert(obj.baz(), "a bar")

    assert(b_called)
    assert(testval, "a bar")
  })

  it("sets self correctly in the super method", ->{
    let self_val = null

    class A {
      foo {
        self_val = self
      }
    }

    class B extends A {
      foo {
        super()
      }
    }

    const obj = new B()
    obj.foo()

    assert(self_val, obj)
  })

  it("copies super and supermember methods", ->{
    class A {
      property name

      foo { @name }
    }

    class B extends A {
      foo { super }
    }

    const obj = new B("myvalue")
    const super_foo = obj.foo()

    assert(typeof super_foo, "function")
    assert(super_foo.name, "foo")
    assert(super_foo(), "myvalue")

    super_foo.bind_self({ name: "some other value" })

    assert(super_foo(), "some other value")
    assert(obj.foo()(), "myvalue")
  })

}
