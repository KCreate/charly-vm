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

class N {
  property value
  property left
  property right
}

const root = new N(
  10,
  new N(7,
    new N(5),
    new N(9)
  ),
  new N(35,
    new N(30,
      new N(20),
      new N(33)
    ),
    new N(45)
  )
)

//               10
//             /    \
//            7     35
//           / \   /  \
//          5  9  30  45
//               /  \
//             20   33

func successor(root, target) {
  let candidate = null
  let base = root

  loop {
    if target < base.value {
      candidate = base.value
      if base.left == null break
      base = base.left
      continue
    }
    if target >= base.value {
      if base.right == null break
      base = base.right
    }
  }

  candidate
}

0.upto(45, ->(i) {
  print(i + ": " + successor(root, i))
})
