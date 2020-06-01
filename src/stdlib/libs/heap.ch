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

/*
 * Base heap class, containing boilerplate methods that don't change between the
 * two different heap types
 * */

class __HeapEntry {
  property weight
  property data
}

const kDefaultCapacity = 16
class __HeapBase {
  property capacity
  property capacity_locked
  property size
  property data

  constructor(@capacity) {
    if capacity <= 0 throw "Heap capacity needs to be at least 1"
    @capacity_locked = false
    @size = 0
    @data = Array.create(@capacity, null)
  }

  // Calculate indices of parent and child nodes
  get_parent_index(n)      = ((n - 1) / 2).floor()
  get_left_child_index(n)  = (n * 2 + 1).floor()
  get_right_child_index(n) = (n * 2 + 2).floor()

  // Check wether parent or child nodes exist
  has_parent(n)            = n > 0
  has_left_child(n)        = @get_left_child_index(n) < @size
  has_right_child(n)       = @get_right_child_index(n) < @size

  // Get parent or child nodes
  get_parent(n)            = @data[@get_parent_index(n)]
  get_left_child(n)        = @data[@get_left_child_index(n)]
  get_right_child(n)       = @data[@get_right_child_index(n)]

  // Make sure we have space for at least one new node
  ensure_capacity {
    if @size == @capacity {
      @data = @data + Array.create(@data.length, null)
      @capacity *= 2
    }
  }

  // Checks if the heap is currently empty
  is_empty = @size == 0

  // Compare two elements using the heap specific comparison operator
  compare_entry(l, r) = @compare(l.weight, r.weight)

  // Checks wether the heap is full
  // This only makes sense when the heap's capacity has been locked
  is_full {
    if !@capacity_locked return false
    return @size == @capacity
  }

  // Returns the first item on the heap or throws if the heap is empty
  peek {
    if @is_empty() throw "Heap is empty"
    @data[0].data
  }

  // Returns the top element of the heap
  poll {
    if @is_empty() throw "Heap is empty"

    const entry = @data[0]
    if @size == 1 {
      @size = 0
      return entry.data
    }

    @data[0] = @data[@size - 1]
    @size -= 1
    @heapify_down()

    entry.data
  }

  // Add a new item to the heap
  //
  // When passed a second argument as the data, $0 will be used as the weight
  push(weight, data = weight) {

    // Special insertion when capacity is locked
    if @capacity_locked && @is_full() {
      if @compare(@data[0].weight, weight) {
        @data[0] = new __HeapEntry(weight, data)
        @heapify_down()
      }
    } else {
      @ensure_capacity()
      @data[@size] = new __HeapEntry(weight, data)
      @size += 1
      @heapify_up()
    }

    self
  }

  // Heapify the heap downwards
  heapify_down {
    let index = 0
    const entry = @data[index]

    while @has_left_child(index) {

      // Get the target child
      //
      // In a min-heap, this would get the smaller child, in a max-heap the bigger one
      let target_child_index = @get_left_child_index(index)
      if @has_right_child(index) && @compare_entry(@get_right_child(index), @get_left_child(index)) {
        target_child_index = @get_right_child_index(index)
      }

      if @compare_entry(@data[index], @data[target_child_index]) {
        break;
      } else {
        @data.swap(index, target_child_index)
      }

      index = target_child_index
    }

    null
  }

  // Heapify the heap upwards
  heapify_up {
    let index = @size - 1
    const entry = @data[index]

    while @has_parent(index) && @compare_entry(entry, @get_parent(index)) {
      const parent_index = @get_parent_index(index)
      @data.swap(index, parent_index)
      index = parent_index
    }

    null
  }
}

class MinHeap extends __HeapBase {
  compare(l, r) = l < r
}

class MaxHeap extends __HeapBase {
  compare(l, r) = l > r
}

export = {

  // Regular heap types
  MinHeap,
  MaxHeap,

  // Heaps of fixed size
  FixedMinHeap: class FixedMinHeap extends MaxHeap {
    constructor {
      @capacity_locked = true
    }
  },
  FixedMaxHeap: class FixedMaxHeap extends MinHeap {
    constructor {
      @capacity_locked = true
    }
  }
}
