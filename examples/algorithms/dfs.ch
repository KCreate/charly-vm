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

    5
   / \
  3   8
 / \ / \
 9 1 7 2
/ \
4 6

*/

class Node {
  property data
  property left
  property right
}

const tree = new Node(
  5,
  new Node(
    3,
    new Node(
      9,
      new Node(4),
      new Node(6)
    ),
    new Node(1)
  ),
  new Node(
    8,
    new Node(7),
    new Node(2)
  )
)

tree.right.__correct = true

let nodes_checked = 0

func search_dfs(node, target) {
  if node == null return null

  nodes_checked += 1

  if node.data == target return node

  let result
  if result = search_dfs(node.left, target) return result
  search_dfs(node.right, target)
}

const result = search_dfs(tree, 8)

print(result.__correct ? "Correct node found" : "Incorrect node found")
print("Checked " + nodes_checked + " nodes")
