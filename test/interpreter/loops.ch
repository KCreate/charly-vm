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

  describe("while", ->{

    it("runs for the specified count", ->{
      let sum = 0
      let index = 0

      while (index < 500) {
        sum += index
        index += 1
      }

      assert(sum, 124750)
    })

    it("breaks a loop", ->{
      let i = 0

      while (true) {
        if i > 99 {
          break
        }

        i += 1
      }

      assert(i, 100)
    })

  })

  describe("continue", ->{

    it("continues to the end of a loop", ->{
      let i = 0
      let sum = 0
      while i < 50 {
        if i % 2 == 0 {
          i += 1
          continue
        }

        sum += 1
        i += 1
      }

      assert(sum, 25)
    })

    it("can be deeply nested", ->{
      let i = 0
      let sum = 0

      while i < 50 {
        if i > 10 {
          if i > 20 {
            if i > 30 {
              i += 1
              continue
            }
          }
        }

        i += 1
        sum += 1
      }

      assert(sum, 31)
    })

  })

  describe("until", ->{

    it("runs a until statement", ->{
      let i = 0

      until i == 100 {
        i += 1
      }

      assert(i, 100)
    })

  })

  describe("loop", ->{

    it("runs a loop statement", ->{
      let i = 0
      loop {
        if i == 100 {
          break
        }

        i += 1
      }
      assert(i, 100)
    })

  })

}
