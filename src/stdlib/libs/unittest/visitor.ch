class TestVisitor {
  on_root(node) {
    print(node.title)
  }

  on_node(node, depth, cb) {
    cb()
    if node.passed() {
      write(".")
    } else {
      write("F")
    }
  }

  on_assertion(index, assertion, depth) {}
}

export = TestVisitor
