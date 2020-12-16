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

export = ->(Base) {
  return class Object extends Base {

    // Empty constructor fallback
    constructor()

    /*
     * Returns an array containing the keys contained inside v
     * */
    static keys(v) {
      __syscall("containerlistkeys", v)
    }

    /*
     * Deletes a key from a type with a container
     * */
    static delete(v, symbol) {
      __syscall("containerdeletekey", v, symbol)
    }

    /*
     * Call the callback with each key, value pair
     * */
    each(cb) {
      Object.keys(self).each(->(key) {
        cb(key, self[key], self)
      })

      self
    }

    /*
     * Create a new object using the return values of the callback
     * function. Callback receives each key, value pair
     * */
    map(cb) {
      const obj = {}

      Object.keys(self).each(->(key) {
        obj[key] = cb(key, self[key], self)
      })

      obj
    }

    /*
     * Create a new object containing only the keys for which
     * the filter callback returned true
     * */
    filter(cb) {
      const obj = {}

      Object.keys(self).each(->(key) {
        const value = self[key]
        if cb(key, value, self) {
          obj[key] = value
        }
      })

      obj
    }
  }
}
