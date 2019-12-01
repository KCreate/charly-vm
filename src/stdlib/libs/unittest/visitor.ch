class TestVisitor {
  func on_root(node) {
    print(node.title)
  }

  func on_node(node, depth, cb) {
    cb()
    if node.passed() {
      write(".")
    } else {
      write("F")
    }
  }

  func on_assertion(index, assertion, depth) {}
}

export = TestVisitor
