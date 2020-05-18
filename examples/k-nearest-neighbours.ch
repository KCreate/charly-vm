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
import "heap"

class Point {
  property x
  property y

  func distance_to(o) {
    const diff_x = math.abs(@x - o.x)
    const diff_y = math.abs(@y - o.y)
    math.sqrt(diff_x ** 2 + diff_y ** 2)
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
const k = 5

// 1. Compute all distances
const points_with_d = points.map(->(p) [p, p.distance_to(target)])

// 2. Store into min-heap
const mh = new heap.MinHeap(points.length)
points_with_d.each(->(p) {
  mh.push(p[1], p)
})

// 3. Return first k elements
const results = []
k.times(->{
  results.push(mh.poll())
})

print("Nearest points:")
results.each(->(p) print(p[0]))











// Time complexity analysis
//
//    Steps                       Time complexity
//
// 1. Compute Distances           O(N)
// 2. Store each into heap        O(N log N)
// 3. Pop K elements from heap    O(K log N)
//
// Result: O(N + (N+K) log N)
//
// Optimisations:
//
// Using a max heap of max-size K would reduce
// the time complexity further down:
//
//    Steps                       Time complexity
//
// 1. Compute Distances           O(N)
// 2. Store each into heap        O(N log K)
// 3. Print heap                  O(K)
//
// Result: O(N + N log K)
//
// The term from step #3 has been dropped as it is as a constant
