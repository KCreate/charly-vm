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
      const close_func = ->n.notify_one()
      spawn(->cb(close_func))

      // Each asynchronous test case has a fixed amount
      // of time to finish before it times out
      const timeout = spawn.promise(->n.error(new Error("Test case timed out")), 100.ms())
      n.wait()
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
