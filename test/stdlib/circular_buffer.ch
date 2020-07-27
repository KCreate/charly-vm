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
  it("read / write to the buffer", ->{
    const c = new CircularBuffer(4)

    c.write(1)
    c.write(2)
    c.write(3)
    c.write(4)

    assert(c.read(), 1)
    assert(c.read(), 2)
    assert(c.read(), 3)
    assert(c.read(), 4)
  })

  it("throws an exception on writes to full buffer", ->{
    const c = new CircularBuffer(4)

    c.write(1)
    c.write(1)
    c.write(1)
    c.write(1)

    const exc = Error.expect(->c.write(1))

    assert(typeof exc, "object")
    assert(exc.message, "Cannot write to full buffer")
  })

  it("throws an exception on reads from empty buffer", ->{
    const c = new CircularBuffer(4)

    const exc = Error.expect(->c.read())

    assert(typeof exc, "object")
    assert(exc.message, "Cannot read from empty buffer")
  })

  it("clears the buffer", ->{
    const c = new CircularBuffer(4)

    c.write(1)
    c.write(2)
    c.write(3)
    c.write(4)

    assert(c.length, 4)

    c.clear()

    assert(c.length, 0)
  })

  it("checks if there is space left in the buffer", ->{
    const c = new CircularBuffer(4)

    assert(c.has_space())

    c.write(1)
    c.write(1)
    c.write(1)
    c.write(1)

    assert(c.has_space(), false)
  })

  it("returns the size of the buffer", ->{
    const c = new CircularBuffer(4)
    assert(c.size(), 4)
  })
}
