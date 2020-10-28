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

  it("inserts the self identifier", ->{
    Number.prototype.cube = func { self ** 3 }
    assert(5.cube(), 125)
  })

  it("has access to the parent scope", ->{
    let count = 0
    Number.prototype.bar = ->{
      count += 1
    }

    1.bar()
    1.bar()
    1.bar()
    1.bar()
    1.bar()

    assert(count, 5)
  })

  it("extends a primitive type", ->{
    Array.prototype.foo = ->"overridden"
    Boolean.prototype.foo = ->"overridden"
    Class.prototype.foo = ->"overridden"
    Function.prototype.foo = ->"overridden"
    Null.prototype.foo = ->"overridden"
    Number.prototype.foo = ->"overridden"
    Object.prototype.foo = ->"overridden"
    String.prototype.foo = ->"overridden"

    assert([].foo(), "overridden")
    assert(false.foo(), "overridden")
    assert((class lol {}).foo(), "overridden")
    assert((func () {}).foo(), "overridden")
    assert(null.foo(), "overridden")
    assert(5.foo(), "overridden")
    assert({}.foo(), "overridden")
    assert("lol".foo(), "overridden")

    Object.delete(Array.prototype, "foo")
    Object.delete(Boolean.prototype, "foo")
    Object.delete(Class.prototype, "foo")
    Object.delete(Function.prototype, "foo")
    Object.delete(Null.prototype, "foo")
    Object.delete(Number.prototype, "foo")
    Object.delete(Object.prototype, "foo")
    Object.delete(String.prototype, "foo")
  })

  it("gives primitive classes the name property", ->{
    assert(Array.name, "Array")
    assert(Boolean.name, "Boolean")
    assert(Class.name, "Class")
    assert(Function.name, "Function")
    assert(Null.name, "Null")
    assert(Number.name, "Number")
    assert(Object.name, "Object")
    assert(String.name, "String")
  })

  it("gives primitive classes the prototype object", ->{
    assert(typeof Array.prototype, "object")
    assert(typeof Boolean.prototype, "object")
    assert(typeof Class.prototype, "object")
    assert(typeof Function.prototype, "object")
    assert(typeof Null.prototype, "object")
    assert(typeof Number.prototype, "object")
    assert(typeof Object.prototype, "object")
    assert(typeof String.prototype, "object")
  })

}
