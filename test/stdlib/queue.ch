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
  it("inserts into the queue", ->{
    const q = new Queue()
    assert(q.length, 0)

    q.write(1)
    q.write(2)
    assert(q.length, 2)

    q.write(3)
    q.write(4)
    assert(q.length, 4)
  })

  it("reads from the queue", ->{
    const q = new Queue()

    q.write(1)
    q.write(2)
    q.write(3)
    q.write(4)

    assert(q.read(1))
    assert(q.read(2))
    assert(q.read(3))
    assert(q.read(4))
  })

  it("gives the length of the queue", ->{
    const q = new Queue()

    assert(q.length, 0)
    q.write(0)
    q.write(0)
    q.write(0)
    q.write(0)
    assert(q.length, 4)

    q.read()
    q.read()
    q.read()
    assert(q.length, 1)

    q.write(1)
    q.write(1)
    q.write(1)
    q.write(1)
    assert(q.length, 5)

    q.read()
    q.read()
    q.read()
    q.read()
    q.read()
    assert(q.length, 0)
  })

  it("throws an exception when trying to read from empty queue", ->{
    const q = new Queue()
    const exc = Error.expect(->q.read())

    assert(exc.message, "Cannot read from empty queue")
  })

  it("clears the queue", ->{
    const q = new Queue()
    q.write(1)
    q.write(1)
    q.write(1)
    q.write(1)

    assert(q.length, 4)

    q.clear()

    assert(q.length, 0)
  })
}
