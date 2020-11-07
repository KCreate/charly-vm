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

export = ->(describe, it, assert) {
  it("static Array.create", ->{
    assert(Array.create(0), [])
    assert(Array.create(4), [null, null, null, null])
    assert(Array.create(-5), [])

    assert(Array.create(4, 25), [25, 25, 25, 25])
    assert(Array.create(4, "test"), ["test", "test", "test", "test"])

    const tmp = [1, 2]
    assert(Array.create(4, tmp), [tmp, tmp, tmp, tmp])

    assert(Array.create(4, ->$0), [0, 1, 2, 3])
  })

  it("copies an array", ->{
    const tmp = [1, 2, 3]
    const original = [1, 2, 3, tmp]
    const copy = original.copy()

    assert(original == copy)

    original[0] = 100
    assert(copy[0], 1)

    assert(original[3] == copy[3])
    assert(original[3][0] == 1)
    assert(copy[3][0] == 1)

    tmp[0] = 200

    assert(original[3][0] == 200)
    assert(copy[3][0] == 200)
  })

  it("iterates over an array", ->{
    const original = [1, 2, 3, 4]
    const result = []

    const each_result = original.each(->(v) result.push(v * v))

    assert(original, each_result)
    assert(result, [1, 4, 9, 16])
  })

  it("maps over an array", ->{
    const original = [1, 2, 3, 4]
    const args = []
    const result = original.map(->(v, i, a) {
      args.push([v, i, a])

      return v * v
    })
    assert(result, [1, 4, 9, 16])
    assert(args, [[1, 0, original], [2, 1, original], [3, 2, original], [4, 3, original]])
  })

  it("reduces an array", ->{
    const original = [1, 2, 3, 4]

    const result = original.reduce(->(last, current, index, array) {
      assert(array == original)
      last + current + index
    }, 0)

    assert(result, 16)
  })

  it("filters an array", ->{
    const original = [1, 2, 3, 4, 5, 6, 7]
    const args = []
    const result = original.filter(->(value, index, array) {
      assert(array == original)
      args.push([value, index])

      if value > 4 return false
      if value % 2 == 0 return false

      true
    })

    assert(args, [[1, 0], [2, 1], [3, 2], [4, 3], [5, 4], [6, 5], [7, 6]])
    assert(result, [1, 3])
  })

  it("joins an array into a string", ->{
    const parts = ["hello", "beautiful", "world"]
    const result = parts.join(" ")

    assert(result, "hello beautiful world")
  })

  it("checks if an array is empty", ->{
    assert([].empty(), true)
    assert([1].empty(), false)
    assert([1, 2].empty(), false)
    assert([null].empty(), false)
  })

  it("pushes an element into the array", ->{
    const arr = []

    arr.push(1)
    arr.push(2)
    arr.push(3)
    arr.push(4)

    assert(arr, [1, 2, 3, 4])
  })

  it("pops an item from the array", ->{
    const arr = [1, 2, 3]

    assert(arr.pop(), 3)
    assert(arr.pop(), 2)
    assert(arr.pop(), 1)
    assert(arr.pop(), null)
  })

  it("appends an element to the front", ->{
    const arr = []

    arr.unshift(1)
    arr.unshift(2)
    arr.unshift(3)
    arr.unshift(4)

    assert(arr, [4, 3, 2, 1])
  })

  it("shifts an element from the array", ->{
    const arr = [1, 2, 3, 4]

    assert(arr.shift(), 1)
    assert(arr.shift(), 2)
    assert(arr.shift(), 3)
    assert(arr.shift(), 4)
    assert(arr.shift(), null)
  })

  it("inserts an element into the array", ->{
    assert([1, 2, 3].insert(-2,  100), [1,   100, 2,   3])
    assert([1, 2, 3].insert(-1,  100), [1,   2,   100, 3])
    assert([1, 2, 3].insert(0,   100), [100, 1,   2,   3])
    assert([1, 2, 3].insert(3,   100), [1,   2,   3,   100])
    assert([1, 2, 3].insert(100, 100), [1,   2,   3])
  })

  it("removes an element from the array", ->{
    assert([1, 2, 3].remove(-1),  [1, 2])
    assert([1, 2, 3].remove(-2),  [1, 3])
    assert([1, 2, 3].remove(0),   [2, 3])
    assert([1, 2, 3].remove(2),   [1, 2])
    assert([1, 2, 3].remove(100), [1, 2, 3])
  })

  it("clears the array", ->{
    assert([].clear(), [])
    assert([1, 2].clear(), [])
    assert([1, 2, 3, 4].clear(), [])
  })

  it("swaps two values inside the array", ->{
    assert([1, 2, 3].swap(0, 1), [2, 1, 3])
    assert([1, 2, 3].swap(0, 0), [1, 2, 3])
    assert([1, 2, 3].swap(0, -1), [3, 2, 1])
  })

  it("iterates over an array via a callback", ->{
    const arr = [1, 2, 3, 4]
    const result = []

    arr.iterate(->(read) {
      result.push(read())
      result.push(read())
      result.push(read())
    })

    assert(result, [1, 2, 3, 4, null, null])
  })

  it("returns the first element of the array", ->{
    assert([].first(), null)
    assert([null].first(), null)
    assert([1, 2, 3].first(), 1)
  })

  it("returns the last element of the array", ->{
    assert([].last(), null)
    assert([null].last(), null)
    assert([1, 2, 3].last(), 3)
  })

  it("reverses an array", ->{
    assert([].reverse(), [])
    assert([1, 2, 3].reverse(), [3, 2, 1])
    assert([1].reverse(), [1])
  })

  it("searches for an item", ->{
    const arr = [1, 2, 3, 2, 1]

    assert(arr.index(1), 0)
    assert(arr.index(3), 2)
    assert(arr.index(5), -1)
  })

  it("searches for an item, reverse", ->{
    const arr = [1, 2, 3, 2, 1]

    assert(arr.rindex(1), 4)
    assert(arr.rindex(3), 2)
    assert(arr.rindex(5), -1)
  })

  it("returns a range of the array", ->{
    const arr = [1, 2, 3, 4]

    assert(arr.range(0, 2), [1, 2])
    assert(arr.range(-2, 4), [3, 4, 1, 2])
    assert(arr.range(2), [3, 4])
  })

  it("checks if the array contains an element", ->{
    const arr = [1, 2, 3, 4, 5, [1, 2, 3]]

    assert(arr.contains(1))
    assert(arr.contains(2))
    assert(arr.contains(3))
    assert(arr.contains([1, 2, 3]))

    assert(arr.contains(null), false)
    assert(arr.contains(25), false)
    assert(arr.contains([1, 2, 4]), false)
    assert(arr.contains(0), false)
  })

  it("checks wether two array are similar", ->{
    const arr = [1, 2, 3, [1, 2, 3]]

    assert(arr.similar([3, 1, [1, 2, 3], 2]))
    assert(arr.similar([3, 1, [1, 2, 3, 4], 2]), false)
    assert(arr.similar([1, [1, 2, 3], 2]), false)
    assert(arr.similar([3, 2, 1, [1, 2, 3], 4]), false)
  })
}
