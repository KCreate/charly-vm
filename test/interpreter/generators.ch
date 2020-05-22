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

  it("creates a generator", ->{
    func foo {
      yield 1
      yield 2
      yield 3
      yield 4
      yield null
    }

    const generator = foo()
    assert(generator(), 1)
    assert(generator(), 2)
    assert(generator(), 3)
    assert(generator(), 4)
    assert(generator(), null)
  })

  it("correctly passes arguments into generator", ->{
    func foo(arg) {
      yield arg
    }

    const generator = foo(5)

    assert(generator(), 5)
    assert(generator(), null)
  })

  it("correctly passes arguments via yield", ->{
    func foo {
      const result1 = yield null
      const result2 = yield result1
      return result2
    }

    const generator = foo()

    assert(generator("hello world"), null)
    assert(generator("test"), "test")
    assert(generator(), null)
  })

  it("correctly sets self inside a generator", ->{
    class Box {
      property data

      func iterator {
        yield self
      }
    }

    const b = new Box([1, 2, 3, 4, 5])
    const it = b.iterator()
    assert(it(), b)
  })

  it("has access to status flags of generator", ->{
    let iter = null

    func foo {
      yield 1
      assert(iter.started, true)
      assert(iter.finished, false)
      assert(iter.running, true)
      yield 2
    }

    iter = foo()

    assert(iter.started, false)
    assert(iter.finished, false)
    assert(iter.running, false)

    assert(iter(), 1)

    assert(iter.started, true)
    assert(iter.finished, false)
    assert(iter.running, false)

    assert(iter(), 2)

    assert(iter.started, true)
    assert(iter.finished, false)
    assert(iter.running, false)

    assert(iter(), null)

    assert(iter.started, true)
    assert(iter.finished, true)
    assert(iter.running, false)
  })

  describe("lambda generators", ->{

    it("immediately starts the generator", ->{
      const gen = ->{
        let i = 0
        loop {
          yield i
          i += 1
        }
      }

      assert(gen.started, false)
      assert(gen.finished, false)
      assert(gen.running, false)

      assert(gen(), 0)
      assert(gen(), 1)
      assert(gen(), 2)
      assert(gen(), 3)
      assert(gen(), 4)
      assert(gen(), 5)
      assert(gen(), 6)
      assert(gen(), 7)

      assert(gen.started, true)
      assert(gen.finished, false)
      assert(gen.running, false)
    })

  })

}
