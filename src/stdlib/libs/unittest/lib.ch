const Context = import "./context.ch"
const TestVisitor = import "./visitor.ch"
const ResultPrinter = import "./results.ch"

// To start a new unit testing session
export = ->(callback) {
  const visitor = new TestVisitor(write, print)
  const context = new Context(visitor)

  callback(
    ->(title, callback) context.suite(title, callback),
    ->(title, callback) context.it(title, callback),
    ->(real, expected) context.assert(real, expected),
    context
  )

  return context.tree
}

// Display the results of a unit testing session
export.display_result = ->(result) {
  const p = new ResultPrinter();
  p.display(result);
}
