const node      = import "./node.ch"
const Node      = node.Node
const NodeType  = node.Type
const Assertion = node.Assertion

class Context {
  property tree
  property current
  property depth
  property visitor

  func constructor(@visitor) {
    @tree = @current = Node("Charly Unit Testing Framework", NodeType.Root)
    @visitor.on_root(@current)
  }

  func add_node(type, title, callback) {
    @depth += 1

    const new_node = Node(title, type)
    const backup = @current
    @current.push(new_node, @depth)
    @current = new_node

    try {
      @visitor.on_node(@current, @depth, callback)
    } catch(e) {
      @catch_exception(e)
    }

    @current = backup

    @depth -= 1
    self
  }

  func suite(title, callback) {
    @add_node(NodeType.Suite, title, callback)
  }

  func it(title, callback) {
    @add_node(NodeType.Test, title, callback)
  }

  func assert(real, expected) {
    const assertion = Assertion(real, expected)

    @current.push(
      assertion,
      @depth
    )

    @visitor.on_assertion(@current.length() - 1, assertion, @depth)
    self
  }

  func catch_exception(e) {
    @assert(e, "No exception to be thrown")
  }
}

export = Context
