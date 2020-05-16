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

  describe("require", ->{

    it("loads an internal module", ->{
      const math = import "math"

      assert(typeof math, "class")
      assert(typeof math.rand, "function")
      assert(typeof math.sin(25), "number")
    })

    it("includes files", ->{
      const module = import "include-test/module.ch"

      assert(module.num, 25)
      assert(typeof module.foo, "function")
      assert(module.foo(), "I am foo")
      assert(module.bar(1, 2), 3)

      let Person = module.Person

      assert(typeof Person, "class")

      let leonard = new Person("Leonard", 2000)
      let bob = new Person("Bob", 1990)

      assert(typeof leonard, "object")
      assert(typeof bob, "object")

      assert(leonard.name, "Leonard")
      assert(leonard.birthyear, 2000)

      assert(leonard.greeting(), "Leonard was born in 2000.")
    })

  })

  describe("stacktraces", ->{

    it("wip", ->{
      const foo = import "include-test/foo.ch"

      try {
        foo.foo(->foo.bar())
      } catch(e) {
        assert(typeof e, "string")
        assert(e, "exc from foo.ch")
      }
    })

  })

}
