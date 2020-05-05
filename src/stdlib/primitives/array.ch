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

const Buffer = import "libs/string/buffer.ch"

export = ->(Base) {
  const _ = {
    insert:  Charly.internals.get_method("PrimitiveArray::insert"),
    remove:  Charly.internals.get_method("PrimitiveArray::remove"),
    reverse: Charly.internals.get_method("PrimitiveArray::reverse"),
    flatten: Charly.internals.get_method("PrimitiveArray::flatten"),
    index:   Charly.internals.get_method("PrimitiveArray::index"),
    rindex:  Charly.internals.get_method("PrimitiveArray::rindex"),
    range:   Charly.internals.get_method("PrimitiveArray::range")
  }

  return class Array extends Base {
    static func create(size, cb) {
      const arr = []

      let i = 0
      while i < size {
        const v = typeof cb == "function" ? cb(i) : cb
        arr << v
        i += 1
      }

      arr
    }

    func copy {
      @map(->$0)
    }

    func each(cb) {
      let i = 0

      while i < @length {
        cb(self[i], i, self)
        i += 1
      }

      self
    }

    func map(cb) {
      let i = 0
      const new_array = []

      while i < @length {
        new_array << cb(self[i], i, self)
        i += 1
      }

      new_array
    }

    func reduce(cb, first) {
      let i = 0
      let sum = first

      while i < @length {
        sum = cb(sum, self[i], i, self)
        i += 1
      }

      sum
    }

    func sum(first) {
      self.reduce(->(l, c) l + c, first)
    }

    func filter(cb) {
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

    func join(str) {
      // TODO: Use some smarter heuristics for the final string length
      const buf = Buffer(@length * 3)
      @each(->(v, i) {
        buf.write(v.to_s())
        if i < @length - 1 {
          buf.write(str.to_s())
        }
      })
      buf.str()
    }

    func empty {
      !@length
    }

    func push(item) {
      self << item
      self
    }

    func pop {
      const item = @last()
      _.remove(self, @length - 1)
      item
    }

    func unshift(item) {
      _.insert(self, item, 0)
      self
    }

    func shift {
      const item = @first()
      _.remove(self, 0)
      item
    }

    func insert(index, item) {
      _.insert(self, index, item)
      self
    }

    func remove(index) {
      _.remove(self, index)
      self
    }

    func clear {
      while @length {
        _.remove(self, 0)
      }
      self
    }

    func iterate(callback) {
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

    func first {
      self[0]
    }

    func last {
      self[@length - 1]
    }

    func reverse {
      _.reverse(self)
    }

    func flatten {
      _.flatten(self)
    }

    func index(element) {
      const offset = $1 || 0
      _.index(self, element, offset)
    }

    func rindex(element) {
      const offset = $1 || -1
      _.rindex(self, element, offset)
    }

    func range(start, count) {
      _.range(self, start, count)
    }

    func contains(search) {
      let i = 0

      while i < @length {
        const value = self[i]
        if (value == search) return true;
        i += 1;
      }

      false
    }
  }
}
