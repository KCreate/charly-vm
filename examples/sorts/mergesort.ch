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

import "math"

func mergesort(arr, start, end) {

  // Single items can't be sorted
  if (start == end) return [arr[start]];

  // If we have two items left, return a sorted version
  if (start + 1 == end) {
    const v1 = arr[start]
    const v2 = arr[end]

    if (v1 < v2) {
      return [v1, v2]
    } else {
      return [v2, v1]
    }
  }

  // Sort left and right half
  const center_pivot = math.floor((start + end) / 2)
  const left_half = mergesort(arr, start, center_pivot - 1)
  const right_half = mergesort(arr, center_pivot, end)

  // Merge the two array
  const result_array = []
  while left_half.length && right_half.length {
    if (left_half[0] < right_half[0]) {
      result_array.push(left_half.shift())
    } else {
      result_array.push(right_half.shift())
    }
  }

  while left_half.length result_array.push(left_half.shift())
  while right_half.length result_array.push(right_half.shift())

  return result_array
}

const numbers = Array.create(20, ->math.rand(0, 100).floor())

print(numbers)

const result = mergesort(numbers, 0, numbers.length - 1)

print(result)
