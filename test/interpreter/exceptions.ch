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
    const cfu = @"charly.vm.write"
    const kla = class A {}

    obj.klass = null

    fun.name = null

    cfu.name = null

    kla.prototype = null
    kla.constructor = null
    kla.name = null
    kla.parent_class = null

    assert(Object.keys(obj).length, 0)
    assert(Object.keys(fun).length, 0)
    assert(Object.keys(cfu).length, 0)
    assert(Object.keys(kla).length, 0)
  })

  it("packs exceptions thrown by the vm into error objects", ->{
    assert.exception(func functionname { const a = @"some undefined symbol" }, ->(e) {
      assert(typeof e, "object")
      assert(e.klass.name, "InternalError")
      assert(e.message, "Unidentified global symbol 'some undefined symbol'")
      assert(typeof e.stacktrace, "array")
      assert(typeof e.stacktrace[0].caller, "function")
      assert(e.stacktrace[0].caller.name, "functionname")
    })

    assert.exception(->{ class A extends null {} }, ->(e) {
      assert(typeof e, "object")
      assert(e.klass.name, "InternalError")
      assert(e.message, "Can't extend from non class value")
    })

    assert.exception(->{ const a = null a() }, ->(e) {
      assert(typeof e, "object")
      assert(e.klass.name, "InternalError")
      assert(e.message, "Attempted to call a non-callable type: null")
    })
  })

  it("rethrows exceptions", ->{
    try {
      try {
        throw new Error("some error happened")
      } catch(e) {
        e.rethrow()
      }
    } catch(e) {
      assert(e.klass, Error)
      assert(e.message, "some error happened")
    }
  })

  it("throws an exception when trying to construct primitive class", ->{
    const err_msg_base = "Cannot construct instance of primitive class: "

    assert.exception(->new Value(),     ->assert($0.message, err_msg_base + "Value"))
    assert.exception(->new Array(),     ->assert($0.message, err_msg_base + "Array"))
    assert.exception(->new Boolean(),   ->assert($0.message, err_msg_base + "Boolean"))
    assert.exception(->new Class(),     ->assert($0.message, err_msg_base + "Class"))
    assert.exception(->new Function(),  ->assert($0.message, err_msg_base + "Function"))
    assert.exception(->new Null(),      ->assert($0.message, err_msg_base + "Null"))
    assert.exception(->new Number(),    ->assert($0.message, err_msg_base + "Number"))
    assert.exception(->new String(),    ->assert($0.message, err_msg_base + "String"))
  })

  describe("Error.expect", ->{
    const exc = Error.expect(->{ throw new Error("some error") })
    assert(typeof exc, "object")
    assert(exc.message, "some error")
  })

}
