const Context = import "./context.ch"
const TestVisitor = import "./visitor.ch"
const ResultPrinter = import "./results.ch"

// To start a new unit testing session
export = ->(callback) {
  const visitor = new TestVisitor(write, print)
  const context = new Context(visitor)

  const assert = ->(real, expected = true) {
    if arguments.length == 1 return context.assert(real)
    context.assert(real, expected)
  }
  assert.exception = ->(cb1, cb2) {
    try {
      cb1()
      assert(null, "An exception to be thrown")
    } catch(e) {
      cb2(e)
    }
  }

  callback(
    ->(title, callback) context.suite(title, callback),
    ->(title, callback) context.it(title, callback),
    assert,
    context
  )

  return context.tree
}

// Display the results of a unit testing session
export.display_result = ->(result) {
  const p = new ResultPrinter();
  p.display(result);
}
