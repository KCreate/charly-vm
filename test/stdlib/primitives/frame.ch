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
  it("returns the current stackframe", ->{
    const top_frame = Frame.current()

    const obj = {}
    obj.foo = func foo { return Frame.current() }

    let bar_frame = null
    func bar {
      bar_frame = Frame.current()
      obj.foo()
    }

    const frame = bar()
    assert(frame.parent, bar_frame)
    assert(frame.parent_environment, top_frame)
    assert(frame.function, obj.foo)
    assert(frame.parent.function, bar)
    assert(frame.self_value, obj)
  })

  it("formats a frame as a string", ->{
    class HostClass {
      testfunc {
        return Frame.current()
      }
    }

    const obj = new HostClass()
    const frame = obj.testfunc()
    assert(frame.to_s(), "<Frame HostClass::testfunc>")

    func foo {
      return Frame.current()
    }
    assert(foo().to_s(), "<Frame foo>")
  })
}
