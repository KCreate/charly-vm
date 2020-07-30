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

const Buffer = import "libs/string/buffer.ch"

const __internal_insert  = @"charly.primitive.array.insert"
const __internal_remove  = @"charly.primitive.array.remove"
const __internal_reverse = @"charly.primitive.array.reverse"
const __internal_flatten = @"charly.primitive.array.flatten"
const __internal_index   = @"charly.primitive.array.index"
const __internal_rindex  = @"charly.primitive.array.rindex"
const __internal_range   = @"charly.primitive.array.range"
const __internal_clear   = @"charly.primitive.array.clear"

export = ->(Base) {
  return class Array extends Base {
    static create(size, cb = null) {
      const arr = []

      let i = 0
      while i < size {
        const v = typeof cb == "function" ? cb(i) : cb
        arr << v
        i += 1
      }

      arr
    }

    copy {
      @map(->$0)
    }

    each(cb) {
      let i = 0

      while i < @length {
        cb(self[i], i, self)
        i += 1
      }

      self
    }

    map(cb) {
      let i = 0
      const new_array = []

      while i < @length {
        new_array << cb(self[i], i, self)
        i += 1
      }

      new_array
    }

    reduce(cb, first) {
      let i = 0
      let sum = first

      while i < @length {
        sum = cb(sum, self[i], i, self)
        i += 1
      }

      sum
    }

    sum(first) {
      self.reduce(->(l, c) l + c, first)
    }

    filter(cb) {
      let i = 0
      const new_array = []

      while i < @length {
        const value = self[i]
        const include = cb(value, i, self)
        if include new_array << value
        i += 1
      }

      new_array
    }

    join(str) {
      // TODO: Use some smarter heuristics for the final string length
      const buf = new Buffer(@length * 3)
      @each(->(v, i) {
        buf.write(v.to_s())
        if i < @length - 1 {
          buf.write(str.to_s())
        }
      })
      buf.to_s()
    }

    empty {
      !@length
    }

    push(item) {
      self << item
      self
    }

    pop {
      const item = @last()
      __internal_remove(self, @length - 1)
      item
    }

    unshift(item) {
      __internal_insert(self, item, 0)
      self
    }

    shift {
      const item = @first()
      __internal_remove(self, 0)
      item
    }

    insert(index, item) {
      __internal_insert(self, index, item)
      self
    }

    remove(index) {
      __internal_remove(self, index)
      self
    }

    clear {
      __internal_clear(self)
      self
    }

    swap(l, r) {
      const tmp = self[l]
      self[l] = self[r]
      self[r] = tmp
      self
    }

    iterate(callback) {
      let i = 0

      func read() {
        let v = self[i]
        i += 1
        v
      }

      while i < @length {
        callback(read)
        if i == 0 break
      }

      self
    }

    first {
      self[0]
    }

    last {
      self[@length - 1]
    }

    reverse {
      __internal_reverse(self)
    }

    flatten {
      __internal_flatten(self)
    }

    index(element) {
      const offset = $1 || 0
      __internal_index(self, element, offset)
    }

    rindex(element) {
      const offset = $1 || -1
      __internal_rindex(self, element, offset)
    }

    range(start, count = @length) {
      __internal_range(self, start, count)
    }

    contains(search) {
      let i = 0

      while i < @length {
        const value = self[i]
        if (value == search) return true
        i += 1
      }

      false
    }
  }
}
