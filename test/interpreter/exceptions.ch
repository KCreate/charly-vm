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

  it("throws an exception", ->{
    try {
      throw "error"
    } catch (e) {
      assert(e, "error")
    }
  })

  it("stops execution of the block", ->{
    try {
      throw 2
      assert(true, false)
    } catch (e) {
      assert(e, 2)
    }
  })

  it("throws exceptions beyond functions", ->{
    func foo() {
      throw "error"
    }

    try {
      foo()
    } catch (e) {
      assert(e, "error")
    }
  })

  it("throws exceptions inside object constructors", ->{
    class Foo {
      constructor() {
        throw "hello world"
      }
    }

    try {
      let a = new Foo()
    } catch (e) {
      assert(e, "hello world")
    }
  })

  it("doesn't write to value container when assigning to reserved fields", ->{
    const obj = {}
    const fun = func foo {}
    const gen = ->{ yield 25 }
    const cfu = @"charly.vm.write"
    const kla = class A {}

    obj.klass = null

    fun.name = null

    gen.finished = null
    gen.started = null
    gen.running = null
    gen.name = null

    cfu.name = null

    kla.prototype = null
    kla.constructor = null
    kla.name = null
    kla.parent_class = null

    assert(Object.keys(obj).length, 0)
    assert(Object.keys(fun).length, 0)
    assert(Object.keys(gen).length, 0)
    assert(Object.keys(cfu).length, 0)
    assert(Object.keys(kla).length, 0)
  })

}
