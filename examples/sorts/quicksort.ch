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

const numbers = Array.create(100, ->Math.rand(0, 100).round())

func swap(a, i1, i2) {
  const tmp = a[i1]
  a[i1] = a[i2]
  a[i2] = tmp
  a
}

func partition(array, pivot, left, right) {
  let store_index = left
  let pivot_value = array[pivot]

  swap(array, pivot, right)

  left.upto(right, ->(index) {
    if (array[index] < pivot_value) {
      swap(array, index, store_index)
      store_index += 1
    }
  })

  swap(array, right, store_index)

  return store_index
}

func sort(array, left = 0, right = array.length - 1) {
  let pivot

  if left < right {
    pivot = left + Math.ceil((right - left) / 2)
    pivot = partition(array, pivot, left, right)
    sort(array, left, pivot - 1)
    sort(array, pivot + 1, right)
  }

  return null
}

print(numbers)
print("")
sort(numbers)
print(numbers)
