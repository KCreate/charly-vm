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

export = ->(describe, it, assert) {

  it("adds numbers", ->{
    assert(2 + 2, 4)
    assert(10 + -50, -40)
    assert(2.5 + 9.7, 12.2)
    assert(-20.5 + 20.5, 0)
    assert(999.999 + 999.999, 1999.998)
  })

  it("subtracts numbers", ->{
    assert(20 - 5, 15)
    assert(3 - 3, 0)
    assert(-5 - 9, -14)
    assert(-0 - -0, 0)
    assert(-20 - -90, 70)
  })

  it("multiplies numbers", ->{
    assert(2 * 0, 0)
    assert(2 * 5, 10)
    assert(3 * 25, 75)
    assert(9 * -50, -450)
    assert(0.5 * 5, 2.5)
  })

  it("divides numbers", ->{
    assert(5 / 0, NaN)
    assert(5 / -2, -2.5)
    assert(10 / 4, 2.5)
    assert(100 / 8, 12.5)
    assert(1 / 2, 0.5)
  })

  it("modulus operator", ->{
    assert(6 % 3, 0)
    assert(0 % 0, NaN)
    assert(177 % 34, 7)
    assert(700 % 200, 100)
    assert(20 % 3, 2)
  })

  it("pow operator", ->{
    assert(2**8, 256)
    assert(50**3, 125000)
    assert(2**4, 16)
    assert(50**1, 50)
    assert(50**0, 1)
  })

  it("does AND assignments", ->{
    let a = 20

    a += 1
    assert(a, 21)

    a -= 1
    assert(a, 20)

    a *= 20
    assert(a, 400)

    a /= 20
    assert(a, 20)

    a %= 6
    assert(a, 2)

    a **= 3
    assert(a, 8)
  })

}
