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

import "math"

export = ->(Base) {
  return class Number extends Base {

    /*
     * Calls the callback *self* times, passing the current iteration count
     * as the first argument
     * */
    func times(cb) {
      let i = 0

      while i < self {
        cb(i)
        i += 1
      }

      null
    }

    /*
     * The same as times, but shorter
     * */
    func n(cb) = @times(cb)

    /*
     * Calls the callback with each number from *self* down to *num*
     * Both *self* and num are inclusive
     * */
    func downto(num, callback) {
      let i = self

      while i >= num {
        callback(i)
        i -= 1
      }

      self
    }

    /*
     * Calls the callback with each number from *self* up to *num*
     * Both *self* and num are inclusive
     * */
    func upto(num, callback) {
      let i = self

      while i <= num {
        callback(i)
        i += 1
      }

      self
    }

    /*
     * Returns the absolute value of this number
     * */
    func abs {
      if self < 0 return -self
      self
    }

    /*
     * Returns the next highest integer
     * */
    func ceil {
      math.ceil(self)
    }

    /*
     * Returns the next lowest integer
     * */
    func floor {
      math.floor(self)
    }

    /*
     * Rounds this number to the nearest integer
     * */
    func round {
      const r = self % 1
      if r < 0.5 return math.floor(self)
      math.ceil(self)
    }

    /*
     * Returns the sign of this number
     * Returns 0 if self is 0
     * */
    func sign {
      if self < 0 return -1
      if self > 0 return 1
      0
    }

    /*
     * Returns the bigger number
     * */
    func max(o) {
      if o > self return o
      self
    }

    /*
     * Returns the smaller number
     * */
    func min(o) {
      if o < self return o
      self
    }

    /*
     * Checks if self is close to *expected* with a given *delta*
     * */
    func close_to(expected, delta) {
      (self - expected).abs() <= delta
    }

    /*
     * Return the number itself
     * */
    func to_n {
      self
    }

    // Converters for different time scales
    func nanosecond   = Charly.time.Duration(self / 1000000)
    func nanoseconds  = Charly.time.Duration(self / 1000000)
    func microsecond  = Charly.time.Duration(self / 1000)
    func microseconds = Charly.time.Duration(self / 1000)
    func millisecond  = Charly.time.Duration(self)
    func milliseconds = Charly.time.Duration(self)
    func second       = Charly.time.Duration(self * 1000)
    func seconds      = Charly.time.Duration(self * 1000)
    func minute       = Charly.time.Duration(self * 1000 * 60)
    func minutes      = Charly.time.Duration(self * 1000 * 60)
    func hour         = Charly.time.Duration(self * 1000 * 60 * 60)
    func hours        = Charly.time.Duration(self * 1000 * 60 * 60)
    func day          = Charly.time.Duration(self * 1000 * 60 * 60 * 24)
    func days         = Charly.time.Duration(self * 1000 * 60 * 60 * 24)
    func week         = Charly.time.Duration(self * 1000 * 60 * 60 * 24 * 7)
    func weeks        = Charly.time.Duration(self * 1000 * 60 * 60 * 24 * 7)
    func year         = Charly.time.Duration(self * 1000 * 60 * 60 * 24 * 365)
    func years        = Charly.time.Duration(self * 1000 * 60 * 60 * 24 * 365)
  }
}
