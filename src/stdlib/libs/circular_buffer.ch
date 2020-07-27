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

// Implements a fixed size circular buffer using an array
// as backing storage
class CircularBuffer {
  property storage
  property length
  property readx
  property writex

  constructor(size = 1) {
    @storage = Array.create(size)
    @length = 0
    @readx = 0
    @writex = 0
  }

  // Write a value to the buffer
  // Will throw if the buffer is full
  write(value) {
    if @length == @size() throw new Error("Cannot write to full buffer")

    // Write to storage
    @storage[@writex] = value
    @writex = (@writex + 1) % @size()
    @length += 1

    null
  }

  // Read a value from the queue or null
  read {
    if @length == 0 throw new Error("Cannot read from empty buffer")

    const value = @storage[@readx]
    @storage[@readx] = null
    @readx = (@readx + 1) % @size()
    @length -= 1

    value
  }

  // Clear the buffer
  clear {
    @readx = @writex
    @length = 0
    @storage.each(->(e, i) {
      @storage[i] = null
    })

    null
  }

  has_space = @length < @size()
  has_data = @length > 0
  size = @storage.length
}

export = CircularBuffer
