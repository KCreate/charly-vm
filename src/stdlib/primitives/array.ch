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

const __internal_insert  = @"charly.primitive.array.insert"
const __internal_remove  = @"charly.primitive.array.remove"
const __internal_reverse = @"charly.primitive.array.reverse"
const __internal_index   = @"charly.primitive.array.index"
const __internal_rindex  = @"charly.primitive.array.rindex"
const __internal_range   = @"charly.primitive.array.range"
const __internal_clear   = @"charly.primitive.array.clear"

export = ->(Base) {
  return class Array extends Base {

    /*
     * Create new array of *size*
     * If *cb* is a function, it calls that function with each index
     * else it uses the value as the initial value
     * */
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

    /*
     * Create a shallow copy of this array
     * */
    copy {
      @map(->$0)
    }

    /*
     * Call the callback with each index, element pair
     * */
    each(cb) {
      let i = 0

      while i < @length {
        cb(self[i], i, self)
        i += 1
      }

      self
    }

    /*
     * Call the callback with each index and element and return
     * a new array with the return values of the callback
     * */
    map(cb) {
      let i = 0
      const new_array = []

      while i < @length {
        new_array << cb(self[i], i, self)
        i += 1
      }

      new_array
    }

    /*
     * Calls the callback with each element and the result of the previous
     * call.
     * *first* is used as the first result value
     * */
    reduce(cb, first) {
      let i = 0
      let sum = first

      while i < @length {
        sum = cb(sum, self[i], i, self)
        i += 1
      }

      sum
    }

    /*
     * Creates a new array with each element not passing the callback test
     * removed
     * */
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

    /*
     * Join the elements together into a string
     * */
    join(str) {
      // TODO: Use some smarter heuristics for the final string length
      const buf = new StringBuffer(@length * 3)
      @each(->(v, i) {
        buf.write(v.to_s())
        if i < @length - 1 {
          buf.write(str.to_s())
        }
      })
      buf.to_s()
    }

    /*
     * Checks wether this array is empty
     * */
    empty {
      @length == 0
    }

    /*
     * Append an element to the arrays
     * */
    push(item) {
      self << item
      self
    }

    /*
     * Removes the last element of the array
     * */
    pop {
      if @empty() return null
      const item = @last()
      __internal_remove(self, @length - 1)
      item
    }

    /*
     * Append to the beginning of the array
     * */
    unshift(item) {
      __internal_insert(self, 0, item)
      self
    }

    /*
     * Removes the first element of the array
     * */
    shift {
      if @empty() return null
      const item = @first()
      __internal_remove(self, 0)
      item
    }

    /*
     * Inserts an element at some index in the array
     * */
    insert(index, item) {
      __internal_insert(self, index, item)
      self
    }

    /*
     * Removes an element at some index
     * */
    remove(index) {
      __internal_remove(self, index)
      self
    }

    /*
     * Clears the contents of the array
     * */
    clear {
      __internal_clear(self)
      self
    }

    /*
     * Swap the values at two indices
     * */
    swap(l, r) {
      const tmp = self[l]
      self[l] = self[r]
      self[r] = tmp
      self
    }

    /*
     * Calls the callback with a function which returns the next
     * element in the array
     *
     * Useful for parsing sequential data
     * */
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

    /*
     * Returns the first element of the array
     * */
    first {
      self[0]
    }

    /*
     * Returns the last element of the array
     * */
    last {
      self[@length - 1]
    }

    /*
     * Returns a reversed copy of the array
     * */
    reverse {
      __internal_reverse(self)
    }

    /*
     * Returns the index of some element in the array, starting at offset
     * Returns -1 if not found
     * */
    index(element, offset = 0) {
      __internal_index(self, element, offset)
    }

    /*
     * Returns the index of some element in the array, starting at offset
     * Starts at the back of the array
     * Returns -1 if not found
     * */
    rindex(element, offset = -1) {
      __internal_rindex(self, element, offset)
    }

    /*
     * Returns a new array containing all the elements in the range of
     * [start, start + count]
     * */
    range(start, count = @length) {
      __internal_range(self, start, count)
    }

    /*
     * Checks wether the array contains an index
     * */
    contains(search) {
      @index(search) ! -1
    }

    /*
     * Checks wether the array contains the same elements as another array,
     * meaning self contains all elements in the other array and the
     * other array contains all elements in self
     * */
    similar(other) {
      unless typeof other == "array" throw new Error("expected argument to be an array")

      // elements of different length are not equal
      if @length ! other.length return false

      // empty arrays are always similar
      if @length == 0 && other.length == 0 return true

      // check if each value contained in self is contained in other
      // and the opposite way around too
      let i = 0
      while i < @length {
        const element = self[i]
        unless other.contains(element) return false
        i += 1
      }

      i = 0
      while i < other.length {
        const element = other[i]
        unless @contains(element) return false
        i += 1
      }

      true
    }
  }
}
