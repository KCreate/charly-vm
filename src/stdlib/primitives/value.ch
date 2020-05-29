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

const __to_s = Charly.internals.get_method("to_s")

export = ->{
  return class Value {
    tap(cb) {
      cb(self)
      self
    }

    pipe() {
      const pipes = arguments

      pipes.each(->(p) {
        guard typeof p == "function" {
          throw ("pipe expected an array of functions, got " + typeof p)
        }

        p(self)
      })

      self
    }

    transform() {
      const pipes = arguments

      let result = self

      pipes.each(->(p) {
        guard typeof p == "function" || typeof p == "cfunction" {
          throw ("pipe expected an array of functions, got " + typeof p)
        }

        result = p(result)
      })

      result
    }

    /*
     * Turn this value into a string
     *
     * Meant to be overridden by other primitive classes
     * */
    to_s {
      __to_s(self)
    }

    /*
     * Turn this value into a number
     *
     * Meant to be overridden by other primitive classes
     * */
    to_n {
      NaN
    }
  }
}
