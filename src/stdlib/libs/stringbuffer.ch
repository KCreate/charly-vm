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
const __stringbuffer_create        = @"charly.stdlib.stringbuffer.create"
const __stringbuffer_reserve       = @"charly.stdlib.stringbuffer.reserve"
const __stringbuffer_get_size      = @"charly.stdlib.stringbuffer.get_size"
const __stringbuffer_get_offset    = @"charly.stdlib.stringbuffer.get_offset"
const __stringbuffer_write         = @"charly.stdlib.stringbuffer.write"
const __stringbuffer_write_partial = @"charly.stdlib.stringbuffer.write_partial"
const __stringbuffer_write_bytes   = @"charly.stdlib.stringbuffer.write_bytes"
const __stringbuffer_to_s          = @"charly.stdlib.stringbuffer.to_s"
const __stringbuffer_bytes         = @"charly.stdlib.stringbuffer.bytes"
const __stringbuffer_clear         = @"charly.stdlib.stringbuffer.clear"

/*
 * Represents a buffer
 *
 * StringBuffers are memory blocks provided by the VM
 * */
class StringBuffer {
  property handle
  property size
  property offset

  constructor(@size = 64) {
    @handle = __stringbuffer_create(size)
    @offset = 0
  }

  /*
   * Make sure a certain amount of *Bytes* are available inside this buffer
   * If multibyte utf8 characters are often used, some heuristic needs to be added
   * to reduce the amount of reallocations in total
   * */
  reserve(size) {
    __stringbuffer_reserve(@handle, size)
    @size = @get_size()
    @offset = @get_offset()
  }

  /*
   * Get the size of the current buffer in bytes and the offset of the write
   * pointer inside that buffer, also in bytes
   * */
  get_size = __stringbuffer_get_size(@handle)
  get_offset = __stringbuffer_get_offset(@handle)

  /*
   * Append the entire content of the src string to the buffer's end
   * */
  write(src) {
    @offset = __stringbuffer_write(@handle, src.to_s())
    @size = @get_size()
  }

  /*
   * Append only parts of a string to the buffer
   * */
  write_partial(src, off, cnt) {
    @offset = __stringbuffer_write_partial(@handle, src, off, cnt)
    @size = @get_size()
  }

  /*
   * Append bytes to the buffer
   * */
  write_bytes(bytes) {
    @offset = __stringbuffer_write_bytes(@handle, bytes)
    @size = @get_size()
  }

  /*
   * Return the string representation of the buffers contents
   * */
  to_s {
    __stringbuffer_to_s(@handle)
  }

  /*
   * Returns the bytes of this string as an int array
   * */
  bytes {
    __stringbuffer_bytes(@handle)
  }

  /*
   * Clear the buffer
   * */
  clear {
    __stringbuffer_clear(@handle)
    @size = @get_size()
    @offset = @get_offset()
  }
}

export = StringBuffer
