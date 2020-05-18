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

import "heap"

export = ->(describe, it, assert) {

  it("creates a min heap", ->{
    const h = new heap.MinHeap(5)

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
    const h = new heap.MaxHeap(5)

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

    const h = new heap.MaxHeap(5)
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



}
