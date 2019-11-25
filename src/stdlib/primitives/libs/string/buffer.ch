/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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

// Internal Method Import
const __buffer_create        = Charly.internals.get_method("Buffer::create")
const __buffer_reserve       = Charly.internals.get_method("Buffer::reserve")
const __buffer_get_size      = Charly.internals.get_method("Buffer::get_size")
const __buffer_get_offset    = Charly.internals.get_method("Buffer::get_offset")
const __buffer_write         = Charly.internals.get_method("Buffer::write")
const __buffer_write_partial = Charly.internals.get_method("Buffer::write_partial")
const __buffer_write_bytes   = Charly.internals.get_method("Buffer::write_bytes")
const __buffer_str           = Charly.internals.get_method("Buffer::str")
const __buffer_bytes         = Charly.internals.get_method("Buffer::bytes")

/*
 * Represents a buffer
 *
 * Buffers are regularly allocated memory blocks provided by the VM
 * */
class Buffer {
  property cp
  property size
  property offset

  func constructor(size) {
    @cp = __buffer_create(size)
    @size = size
    @offset = 0
  }

  /*
   * Make sure a certain amount of *Bytes* are available inside this buffer
   * If multibyte utf8 characters are often used, some heuristic needs to be added
   * to reduce the amount of reallocations in total
   * */
  func reserve(size) {
    __buffer_reserve(@cp, size)
    @size = @get_size()
    @offset = @get_offset()
  }

  /*
   * Get the size of the current buffer in bytes and the offset of the write
   * pointer inside that buffer, also in bytes
   * */
  func get_size = __buffer_get_size(@cp)
  func get_offset = __buffer_get_offset(@cp)

  /*
   * Append the entire content of the src string to the buffer's end
   * */
  func write(src) {
    @offset = __buffer_write(@cp, src)
    @size = @get_size()
  }

  /*
   * Append only parts of a string to the buffer
   * */
  func write_partial(src, off, cnt) {
    @offset = __buffer_write_partial(@cp, src, off, cnt)
    @size = @get_size()
  }

  /*
   * Append the entire content of the src string to the buffer's end
   * */
  func write_bytes(bytes) {
    @offset = __buffer_write_bytes(@cp, bytes)
    @size = @get_size()
  }

  /*
   * Return the string representation of the buffers contents
   * */
  func str {
    __buffer_str(@cp)
  }

  /*
   * Returns the bytes of this string as an int array
   * */
  func bytes {
    __buffer_bytes(@cp)
  }
}

export = Buffer
