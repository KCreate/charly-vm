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
  return class String extends Base {

    /*
     * Returns an all lowercase copy of this string
     * */
    lowercase {
      __syscall("stringlowercase", self)
    }

    /*
     * Returns an all uppercase copy of this string
     * */
    uppercase {
      __syscall("stringuppercase", self)
    }

    /*
     * Trim whitespace off the left side of the string
     * */
    ltrim {
      __syscall("stringtriml", self)
    }

    /*
     * Trim whitespace off the right side of the string
     * */
    rtrim {
      __syscall("stringtrimr", self)
    }

    /*
     * Trim whitespace off both sides of the string
     * */
    trim {
      @ltrim().rtrim()
    }

    /*
     * Returns a new string which is a substring
     * starting at *start* with a length of *offset*
     *
     * ```
     * "hello world".substring(5, 5) // => "world"
     * ```
     * */
    substring(start, offset = @length) {
      if offset == 0 return ""
      const buf = new StringBuffer((offset == @length) ? (@length - start) : (offset))

      if start < 0 {
        start += @length
        if start < 0 return ""
      }

      buf.write_partial(self, start, offset)

      buf.to_s()
    }

    /*
     * Returns a reversed copy of this string
     * */
    reverse {
      const buf = new StringBuffer(@length)
      const end = @length - 1
      end.downto(0, ->(i) {
        buf.write(self[i])
      })
      buf.to_s()
    }

    /*
     * Calls the callback with each character in this string
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
     * Returns a new string where each char is replaced by the value
     * returned for it by the callback
     * */
    map(cb) {
      let i = 0
      let buf = new StringBuffer(@length)

      while i < @length {
        buf.write(cb(self[i], i, self))
        i += 1
      }

      buf.to_s()
    }

    /*
     * Returns a new string where each char, for which the cb returned false, is removed
     * */
    filter(cb) {
      let i = 0
      let buf = new StringBuffer(@length)

      while i < @length {
        const r = cb(self[i], i, self)
        if r buf.write(self[i])
        i += 1
      }

      buf.to_s()
    }

    /*
     * Returns an array of strings by splitting the original at every
     * occurence of the *needle*
     *
     * ```
     * "hello world whats up".split(" ") // => ["hello", "world", "whats", "up"]
     * "whats up".split("") // => ["h", "e", "l", "l", "o", " ", "w", "o", "r", "l", "d"]
     * ```
     * */
    split(needle) {

      // If the needle is the empty string, we split after each utf8 codepoint
      if needle == "" {
        const r = []
        self.each(->(c) r << c)
        return r
      }

      let result = []

      let i = 0
      let last_marker = 0

      while i <= @length {
        const sub = @substring(i, needle.length)

        if sub == needle || i == @length {
          result << @substring(last_marker, i - last_marker)
          i += needle.length
          last_marker = i
          continue
        }

        i += 1
      }

      result
    }

    /*
     * Returns the index of a substring
     * */
    index(target, offset = 0) {
      if target.length > @length return -1

      if offset < 0 offset += @length
      if offset < 0 return -1
      let i = offset

      while i + target.length <= @length {
        const sub = @substring(i, target.length)
        if sub == target return i
        i += 1
      }

      -1
    }

    /*
     * Returns the index of a substring starting at the back of the string
     * */
    rindex(target, offset = @length) {
      if target.length > @length return -1

      if offset < 0 offset += @length
      if offset < 0 return -1
      let i = offset - target.length

      while i > 0 {
        const sub = @substring(i, target.length)
        if sub == target return i
        i -= 1
      }

      -1
    }

    /*
     * Checks wether this string contains another string
     * */
    contains(target) {
      const index = @index(target)
      if index == -1 return false
      true
    }

    /*
     * Checks wether this string begins with another string
     * */
    begins_with(target) {
      const index = @index(target)
      if index == -1 return false
      if index == 0 return true
      false
    }

    /*
     * Checks wether this string ends with another string
     * */
    ends_with(target) {
      const index = @index(target)
      if index == -1 return false
      if index == @length - target.length return true
      false
    }

    /*
     * Checks if this string is a single digit
     *
     * ```
     * "5".is_digit() // => true
     * "2".is_digit() // => true
     * "0".is_digit() // => true
     * "f".is_digit() // => false
     * ```
     * */
    is_digit {
      const r = ({
        @"0": true,
        @"1": true,
        @"2": true,
        @"3": true,
        @"4": true,
        @"5": true,
        @"6": true,
        @"7": true,
        @"8": true,
        @"9": true
      })[self]
      r || false
    }

    /*
     * Convert this string into a number
     *
     * ```
     * "2562".to_n() // => 2562
     * "-2562".to_n() // => -2562
     * "25.5".to_n() // => 25.5
     * "-1.88".to_n() // => -1.88
     * "test".to_n() // => NaN
     * ```
     * */
    to_n {
      __syscall("castnumber", self.trim())
    }

    /*
     * Returns the first character in this string
     * */
    first {
      self[0]
    }

    /*
     * Returns the last character in this string
     * */
    last {
      self[@length - 1]
    }
  }
}
