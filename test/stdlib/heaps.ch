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

  it("creates a min heap", ->{
    const h = new Heap.MinHeap(5)

    h.push(3)
    h.push(9)
    h.push(4)
    h.push(-2)
    h.push(0)

    assert(h.is_empty(), false)

    assert(h.peek(), -2)
    assert(h.poll(), -2)

    assert(h.peek(), 0)
    assert(h.poll(), 0)

    assert(h.peek(), 3)
    assert(h.poll(), 3)

    assert(h.peek(), 4)
    assert(h.poll(), 4)

    assert(h.peek(), 9)
    assert(h.poll(), 9)

    assert(h.is_empty(), true)
  })

  it("creates a max heap", ->{
    const h = new Heap.MaxHeap(5)

    h.push(3)
    h.push(9)
    h.push(4)
    h.push(-2)
    h.push(0)

    assert(h.is_empty(), false)

    assert(h.peek(), 9)
    assert(h.poll(), 9)

    assert(h.peek(), 4)
    assert(h.poll(), 4)

    assert(h.peek(), 3)
    assert(h.poll(), 3)

    assert(h.peek(), 0)
    assert(h.poll(), 0)

    assert(h.peek(), -2)
    assert(h.poll(), -2)

    assert(h.is_empty(), true)
  })

  it("uses separate weight and data values", ->{
    class Num {
      property data
    }

    const h = new Heap.MaxHeap(5)
    h.push(1, new Num(1))
    h.push(4, new Num(2))
    h.push(3, new Num(3))
    h.push(5, new Num(4))
    h.push(2, new Num(5))

    assert(h.is_empty(), false)

    assert(h.peek().data, 4)
    assert(h.poll().data, 4)

    assert(h.peek().data, 2)
    assert(h.poll().data, 2)

    assert(h.peek().data, 3)
    assert(h.poll().data, 3)

    assert(h.peek().data, 5)
    assert(h.poll().data, 5)

    assert(h.peek().data, 1)
    assert(h.poll().data, 1)

    assert(h.is_empty(), true)
  })

  it("throws an exception on invalid capacity values", ->{
    assert.exception(->{
      const h = new Heap.MaxHeap(0)
    }, ->(e) {
      assert(typeof e, "string")
      assert(e, "Heap capacity needs to be at least 1")
    })

    assert.exception(->{
      const h = new Heap.MaxHeap(-20)
    }, ->(e) {
      assert(typeof e, "string")
      assert(e, "Heap capacity needs to be at least 1")
    })
  })

  describe("heaps of fixed size", ->{
    it("min heap", ->{
      const minh = new Heap.FixedMinHeap(3)

      assert(minh.is_full(), false)
      assert(minh.size, 0)

      minh.push(3)
      minh.push(4)
      minh.push(5)

      assert(minh.is_full(), true)
      assert(minh.size, 3)

      minh.push(2)
      minh.push(1)

      assert(minh.is_full(), true)
      assert(minh.size, 3)

      minh.push(10)
      minh.push(12)
      minh.push(14)

      assert(minh.is_full(), true)
      assert(minh.size, 3)

      // Because max-sized heaps are implemented using the opposite heap type
      // the elements are popped off in reverse order (biggest to smallest)
      assert(minh.poll(), 3)
      assert(minh.poll(), 2)
      assert(minh.poll(), 1)

      assert(minh.is_full(), false)
      assert(minh.size, 0)
    })

    it("max heap", ->{
      const maxh = new Heap.FixedMaxHeap(3)

      assert(maxh.is_full(), false)
      assert(maxh.size, 0)

      maxh.push(10)
      maxh.push(9)
      maxh.push(8)

      assert(maxh.is_full(), true)
      assert(maxh.size, 3)

      maxh.push(11)
      maxh.push(12)

      assert(maxh.is_full(), true)
      assert(maxh.size, 3)

      maxh.push(3)
      maxh.push(2)
      maxh.push(1)

      assert(maxh.is_full(), true)
      assert(maxh.size, 3)

      // Because max-sized heaps are implemented using the opposite heap type
      // the elements are popped off in reverse order (smallest to biggest)
      assert(maxh.poll(), 10)
      assert(maxh.poll(), 11)
      assert(maxh.poll(), 12)

      assert(maxh.is_full(), false)
      assert(maxh.size, 0)
    })
  })


}
