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
  it("creates a stringbuffer", ->{
    const buf = new StringBuffer(64)
    assert(typeof buf, "object")
    assert(buf.get_size(), 64)
    assert(buf.get_offset(), 0)
    assert(typeof buf.handle, "cpointer")
  })

  it("reserves size", ->{
    const buf = new StringBuffer(64)

    buf.reserve(128)
    assert(buf.get_size(), 128)
    assert(buf.get_offset(), 0)

    buf.reserve(256)
    assert(buf.get_size(), 256)
    assert(buf.get_offset(), 0)

    buf.reserve(512)
    assert(buf.get_size(), 512)
    assert(buf.get_offset(), 0)
  })

  it("writes a string", ->{
    const buf = new StringBuffer(64)

    buf.write("hello world this is a test hello")

    assert(buf.get_size(), 64)
    assert(buf.get_offset(), 32)

    buf.write("some more data")

    assert(buf.get_size(), 64)
    assert(buf.get_offset(), 46)

    buf.write("some more data")

    assert(buf.get_size(), 64)
    assert(buf.get_offset(), 60)

    buf.write("some more data")

    assert(buf.get_size(), 128)
    assert(buf.get_offset(), 74)
  })

  it("writes a partial string", ->{
    const buf = new StringBuffer(64)

    buf.write_partial("hello world, test string", 6, 5)

    assert(buf.get_size(), 64)
    assert(buf.get_offset(), 5)

    buf.write_partial("hello world, test string", 0, 5)

    assert(buf.get_size(), 64)
    assert(buf.get_offset(), 10)

    buf.write_partial("hello world, test string", 13, 11)

    assert(buf.get_size(), 64)
    assert(buf.get_offset(), 21)

    assert(buf.to_s(), "worldhellotest string")
  })

  it("writes an array of bytes", ->{
    const buf = new StringBuffer(64)

    // Hello World this is a test
    const bytes = [
      72, 101, 108, 108, 111, 32, 87, 111, 114, 108, 100, 32, 116,
      104, 105, 115, 32, 105, 115, 32, 97, 32, 116, 101, 115, 116
    ]

    buf.write_bytes(bytes)

    assert(buf.get_size(), 64)
    assert(buf.get_offset(), bytes.length)
    assert(buf.to_s(), "Hello World this is a test")
  })

  it("returns an array of bytes", ->{
    const buf = new StringBuffer(64)

    buf.write("Hello World this is a test")

    const bytes = [
      72, 101, 108, 108, 111, 32, 87, 111, 114, 108, 100, 32, 116,
      104, 105, 115, 32, 105, 115, 32, 97, 32, 116, 101, 115, 116
    ]

    assert(buf.get_size(), 64)
    assert(buf.get_offset(), 26)
    assert(buf.bytes(), bytes)
  })

  it("clears the buffer", ->{
    const buf = new StringBuffer(64)

    buf.write("hello world this is a test")

    assert(buf.get_size(), 64)
    assert(buf.get_offset(), 26)

    buf.clear()

    assert(buf.get_size(), 64)
    assert(buf.get_offset(), 0)
  })
}
