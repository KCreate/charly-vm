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

const __rand = Charly.internals.get_method("Math::rand")

class Math {
  func constructor {
    throw "Cannot initialize an instance of the Math class"
  }

  static property PI    = 3.14159265358979323846
  static property E     = 2.7182818284590451
  static property LOG2  = 0.69314718055994529
  static property LOG10 = 2.3025850929940459

  static property cos   = Charly.internals.get_method("Math::cos")
  static property cosh  = Charly.internals.get_method("Math::cosh")
  static property acos  = Charly.internals.get_method("Math::acos")
  static property acosh = Charly.internals.get_method("Math::acosh")

  static property sin   = Charly.internals.get_method("Math::sin")
  static property sinh  = Charly.internals.get_method("Math::sinh")
  static property asin  = Charly.internals.get_method("Math::asin")
  static property asinh = Charly.internals.get_method("Math::asinh")

  static property tan   = Charly.internals.get_method("Math::tan")
  static property tanh  = Charly.internals.get_method("Math::tanh")
  static property atan  = Charly.internals.get_method("Math::atan")
  static property atanh = Charly.internals.get_method("Math::atanh")

  static property cbrt  = Charly.internals.get_method("Math::cbrt")
  static property sqrt  = Charly.internals.get_method("Math::sqrt")
  static property hypot = ->(x, y) Math.sqrt(x ** 2, y ** 2)

  static property ceil  = Charly.internals.get_method("Math::ceil")
  static property floor = Charly.internals.get_method("Math::floor")

  static property log   = Charly.internals.get_method("Math::log")
  static property log2  = Charly.internals.get_method("Math::log2")
  static property log10 = Charly.internals.get_method("Math::log10")

  /*
   * Returns a random value depending on the arguments passed
   *
   * 0 args: 0 - 1
   * 1  arg: 0 - x
   * 2 args: x - y
   * */
  static func rand {
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
  static func abs(n) {
    if n < 0 return -n
    n
  }
}

export = Math
