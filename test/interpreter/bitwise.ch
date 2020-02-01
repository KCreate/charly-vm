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

  const TEST_NUMS = [
    [-50, -20],
    [-20, 50],
    [50, -2],
    [1, 2],
    [3, 4],
    [5, 6],
    [7, 8]
  ]

  const TEST_SINGLE_NUMS = [
    10,
    20,
    30,
    -20,
    -30,
    -40,
    0
  ]

  const TEST_SHIFT_NUMS = [
    [32, 4],
    [1, 16],
    [-20, 5],
    [16, -2],
    [8, -4]
  ]

  describe("AND", ->{

    it("performs bitwise AND", ->{
      let results = TEST_NUMS.map(->(pair) pair[0] & pair[1])

      assert(results, [
        -52,
        32,
        50,
        0,
        0,
        4,
        0
      ])
    })

  })

  describe("OR", ->{

    it("performs bitwise OR", ->{
      let results = TEST_NUMS.map(->(pair) pair[0] | pair[1])

      assert(results, [
        -18,
        -2,
        -2,
        3,
        7,
        7,
        15
      ])
    })

  })

  describe("XOR", ->{

    it("performs bitwise XOR", ->{
      let results = TEST_NUMS.map(->(pair) pair[0] ^ pair[1])

      assert(results, [
        34,
        -34,
        -52,
        3,
        7,
        3,
        15
      ])
    })

  })

  describe("NOT", ->{

    it("performs bitwise NOT", ->{
      let results = TEST_SINGLE_NUMS.map(->(num) ~num)

      assert(results, [
        -11,
        -21,
        -31,
        19,
        29,
        39,
        -1
      ])
    })

  })

  describe("left-shift", ->{

    it("performs a left shift", ->{
      let results = TEST_SHIFT_NUMS.map(->(pair) pair[0] << pair[1])

      assert(results, [
        512,
        65536,
        -640,
        16,
        8
      ])
    })

  })

  describe("right-shift", ->{

    it("performs a right shift", ->{
      let results = TEST_SHIFT_NUMS.map(->(pair) pair[0] >> pair[1])

      assert(results, [
        2,
        0,
        -1,
        16,
        8
      ])
    })

  })

}
