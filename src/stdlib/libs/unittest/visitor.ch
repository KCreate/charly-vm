class TestVisitor {
  on_root(node) {
    print(node.title)
  }

  on_node(node, depth, cb) {
    // If the callback takes at least one argument, we assume the test case will
    // run something asynchronously, so we provide a done callback which wraps
    // the asynchronous flow back into the synchronous flow of this method
    if cb.argc == 1 {
      const n = new Sync.Notifier()
      defer(->cb(->n.close()))
      const timeout = defer(->n.error(new Error("Test case timed out")), 100.ms())
      const exc = n.wait()
      if exc throw exc
      timeout.clear()
    } else {
      cb()
    }

    if node.passed() {
      write(".")
    } else {
      write("F")
    }
  }

  on_assertion(index, assertion, depth) {}
}

export = TestVisitor
