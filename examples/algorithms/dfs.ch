/*

    5
   / \
  3   8
 / \ / \
 9 1 7 2

*/

class Node {
  property data
  property left
  property right
}

const target_node = Node(7)
target_node.__correct = true

const tree = Node(
  5,
  Node(
    3,
    Node(9),
    Node(1)
  ),
  Node(
    8,
    target_node,
    Node(2)
  )
)

func search_dfs(node, target) {
  if node == null return null
  if node.data == target return node

  let result
  if result = search_dfs(node.left, target) return result
  search_dfs(node.right, target)
}

const result = search_dfs(tree, 7)

print(result)

print(result.__correct ? "Correct node found" : "Incorrect node found")
