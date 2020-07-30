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

// Asynchronous communication between charly threads
class Channel {
  property open
  property buffer
  property pending_read
  property pending_write

  constructor(size = 0) {
    @open = true
    @buffer = size ? new CircularBuffer(size) : null
    @pending_read = new Sync.Notifier()
    @pending_write = new Sync.Notifier()
  }

  read {
    if @buffer && @buffer.length {
      const value = @buffer.read()

      if @pending_write.has_waiters() {
        @pending_write.notify_one()
      }

      return value
    }

    unless @open return null

    if @pending_write.has_waiters() {
      @pending_write.notify_one()
    }

    return @pending_read.wait()
  }

  write(value) {
    unless @open throw new Error("Cannot write to a closed channel")

    // Check pending reads
    if @pending_read.has_waiters() {
      @pending_read.notify_one(value)
      return null
    }

    // Append to queue if there are still spots left
    if @buffer && @buffer.has_space() {
      @buffer.write(value)
      return null
    }

    // Append to writer waitlist
    @pending_write.wait()
    @write(value)

    null
  }

  close {
    unless @open throw new Error("Channel already closed")
    @open = false
    @pending_read.close()
    @pending_write.error(new Error("Write cancelled, channel was closed"))
  }
}

export = Channel
