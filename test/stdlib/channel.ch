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
  describe("Unbuffered channels", ->{
    it("writing to channel blocks", ->(done) {
      const c = new Sync.Channel()
      let check = false

      spawn(->{
        assert(check, false)
        assert(c.read(), "test")
        Sync.sleep(0)
        assert(check, true)

        done()
      })

      c.write("test")
      check = true
    })

    it("reading from channel blocks", ->(done) {
      const c = new Sync.Channel()
      let check = false

      spawn(->{
        assert(check, false)
        c.write("test")
        Sync.sleep(0)
        assert(check, true)

        done()
      })

      assert(c.read(), "test")
      check = true
    })

    it("closing a channel", ->{
      const c = new Sync.Channel()

      assert(c.open, true)
      c.close()
      assert(c.open, false)
    })

    it("read from closed channel returns null", ->{
      const c = new Sync.Channel()
      c.close()

      assert(c.read(), null)
      assert(c.read(), null)
      assert(c.read(), null)
    })

    it("outstanding reads return null when channel is closed", ->{
      const c = new Sync.Channel()

      spawn(->c.close())

      assert(c.open, true)

      const msg = c.read()
      assert(msg, null)
      assert(c.open, false)
    })

    it("outstanding writes throw when channel is closed", ->{
      const c = new Sync.Channel()

      spawn(->c.close())

      const exc = Error.expect(->c.write("testvalue"))
      assert(exc.message, "Write cancelled, channel was closed")
    })

    it("exception on write to closed channel", ->{
      const c = new Sync.Channel()

      c.close()

      const exc = Error.expect(->c.write("testvalue"))
      assert(exc.message, "Cannot write to a closed channel")
    })

    it("exception on closing already closed channel", ->{
      const c = new Sync.Channel()
      c.close()

      const exc = Error.expect(->c.close())
      assert(exc.message, "Channel already closed")
    })
  })

  describe("Buffered channels", ->{
    it("being able to write consecutive values without blocking", ->(done) {
      const c = new Sync.Channel(4)
      let check = false

      spawn(->{
        assert(check, true)
        assert(c.open, false)

        done()
      })

      c.write(1)
      c.write(2)
      c.write(3)
      c.write(4)
      c.close()
      check = true
    })

    it("being able to read N consecutive values without blocking", ->(done) {
      const c = new Sync.Channel(4)
      let check = false

      spawn(->{
        assert(check, true)
        assert(c.open, false)

        done()
      })

      c.write(1)
      c.write(2)
      c.write(3)
      c.write(4)
      assert(c.read(), 1)
      assert(c.read(), 2)
      assert(c.read(), 3)
      assert(c.read(), 4)
      c.close()
      check = true
    })

    it("reading remaining buffer from closed channel", ->(done) {
      const c = new Sync.Channel(4)
      let check = false

      spawn(->{
        assert(check, true)
        assert(c.open, false)

        assert(c.read(), 1)
        assert(c.read(), 2)
        assert(c.read(), 3)
        assert(c.read(), 4)
        assert(c.read(), null)

        done()
      })

      c.write(1)
      c.write(2)
      c.write(3)
      c.write(4)
      c.close()
      assert(c.open, false)
      check = true
    })
  })
}
