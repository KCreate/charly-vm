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

class Point {
  property x
  property y

  distance_to(o) {
    const diff_x = Math.abs(@x - o.x)
    const diff_y = Math.abs(@y - o.y)
    Math.sqrt(diff_x ** 2 + diff_y ** 2)
  }
}

const points = [
  new Point(2, 2),
  new Point(2, 3),
  new Point(2, -1),
  new Point(-5, -2),
  new Point(1, 0)
]

const target = new Point(5, -4)
const k = 3

// 1. Compute all distances and store into a fixed size min heap
const h = new heap.FixedMinHeap(k)
points.map(->(p) {

  // Arg #1 is the weight, #2 is the data
  h.push(p.distance_to(target), p)
})

// 2. Get the smallest k points
print("Nearest points:")
while !h.is_empty() {
  print(h.poll())
}

// Time complexity analysis
//
//    Steps                       Time complexity
//
// 1. Compute Distances           O(N)
// 2. Store each into heap        O(N log K)
// 3. Pop each element            O(K log K)
//
// Depending on wether you want to count the removal of each element for printing
// as part of the algorithm the time complexity is either
//
// With:      O((N + K) log K)
// Without:   O(N log K)
//
// The first O(N) term is dropped, as the second one dominates it
