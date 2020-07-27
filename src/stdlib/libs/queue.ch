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

class QueueEntry {
  property data
  property next
}

class Queue {
  property head
  property tail
  property length

  constructor {
    @head = null
    @tail = null
    @length = 0
  }

  read {
    if @length == 0 throw new Error("Cannot read from empty queue")

    if @length == 1 {
      const front = @head
      @clear()
      return front.data
    }

    const front = @head
    @head = front.next
    @length -= 1
    return front.data
  }

  write(value) {
    if @length == 0 {
      @head = @tail = new QueueEntry(value, null)
    } else {
      const entry = new QueueEntry(value, null)
      @tail.next = entry
      @tail = entry
    }

    @length += 1

    null
  }

  clear {
    @head = null
    @tail = null
    @length = 0
  }
}

export = Queue
