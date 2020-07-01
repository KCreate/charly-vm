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

const __rand = @"charly.stdlib.math.rand"

class Math {
  constructor {
    throw "Cannot initialize an instance of the Math class"
  }

  static property PI    = 3.14159265358979323846
  static property E     = 2.7182818284590451
  static property LOG2  = 0.69314718055994529
  static property LOG10 = 2.3025850929940459

  static property cos   = @"charly.stdlib.math.cos"
  static property cosh  = @"charly.stdlib.math.cosh"
  static property acos  = @"charly.stdlib.math.acos"
  static property acosh = @"charly.stdlib.math.acosh"

  static property sin   = @"charly.stdlib.math.sin"
  static property sinh  = @"charly.stdlib.math.sinh"
  static property asin  = @"charly.stdlib.math.asin"
  static property asinh = @"charly.stdlib.math.asinh"

  static property tan   = @"charly.stdlib.math.tan"
  static property tanh  = @"charly.stdlib.math.tanh"
  static property atan  = @"charly.stdlib.math.atan"
  static property atanh = @"charly.stdlib.math.atanh"

  static property cbrt  = @"charly.stdlib.math.cbrt"
  static property sqrt  = @"charly.stdlib.math.sqrt"
  static property hypot = ->(x, y) Math.sqrt(x ** 2, y ** 2)

  static property ceil  = @"charly.stdlib.math.ceil"
  static property floor = @"charly.stdlib.math.floor"

  static property log   = @"charly.stdlib.math.log"
  static property log2  = @"charly.stdlib.math.log2"
  static property log10 = @"charly.stdlib.math.log10"

  /*
   * Round number to nearest integer
   * */
  static round(n) {
    const rem = n % 1
    if rem < 0.5 return Math.floor(n)
    if rem >= 0.5 return Math.ceil(n)
  }

  /*
   * Returns a random value depending on the arguments passed
   *
   * 0 args: 0 - 1
   * 1  arg: 0 - x
   * 2 args: x - y
   * */
  static rand {
    switch arguments.length {
      case 1 {
        return __rand(0, $0)
      }

      case 2 {
        return __rand($0, $1)
      }

      default {
        return __rand(0, 1)
      }
    }
  }

  /*
   * Returns the absolute value of a number
   * */
  static abs(n) {
    if n < 0 return -n
    n
  }
}

export = Math
