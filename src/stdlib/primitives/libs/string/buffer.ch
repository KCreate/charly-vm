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

// Internal Method Import
const __buffer_create        = @"charly.stdlib.buffer.create"
const __buffer_reserve       = @"charly.stdlib.buffer.reserve"
const __buffer_get_size      = @"charly.stdlib.buffer.get_size"
const __buffer_get_offset    = @"charly.stdlib.buffer.get_offset"
const __buffer_write         = @"charly.stdlib.buffer.write"
const __buffer_write_partial = @"charly.stdlib.buffer.write_partial"
const __buffer_write_bytes   = @"charly.stdlib.buffer.write_bytes"
const __buffer_str           = @"charly.stdlib.buffer.str"
const __buffer_bytes         = @"charly.stdlib.buffer.bytes"

/*
 * Represents a buffer
 *
 * Buffers are regularly allocated memory blocks provided by the VM
 * */
class Buffer {
  property handle
  property size
  property offset

  constructor(@size) {
    @handle = __buffer_create(size)
    @offset = 0
  }

  /*
   * Make sure a certain amount of *Bytes* are available inside this buffer
   * If multibyte utf8 characters are often used, some heuristic needs to be added
   * to reduce the amount of reallocations in total
   * */
  reserve(size) {
    __buffer_reserve(@handle, size)
    @size = @get_size()
    @offset = @get_offset()
  }

  /*
   * Get the size of the current buffer in bytes and the offset of the write
   * pointer inside that buffer, also in bytes
   * */
  get_size = __buffer_get_size(@handle)
  get_offset = __buffer_get_offset(@handle)

  /*
   * Append the entire content of the src string to the buffer's end
   * */
  write(src) {
    @offset = __buffer_write(@handle, src)
    @size = @get_size()
  }

  /*
   * Append only parts of a string to the buffer
   * */
  write_partial(src, off, cnt) {
    @offset = __buffer_write_partial(@handle, src, off, cnt)
    @size = @get_size()
  }

  /*
   * Append bytes to the buffer
   * */
  write_bytes(bytes) {
    @offset = __buffer_write_bytes(@handle, bytes)
    @size = @get_size()
  }

  /*
   * Return the string representation of the buffers contents
   * */
  str {
    __buffer_str(@handle)
  }

  /*
   * Returns the bytes of this string as an int array
   * */
  bytes {
    __buffer_bytes(@handle)
  }
}

export = Buffer
