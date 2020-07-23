const node      = import "./node.ch"
const Node      = node.Node
const NodeType  = node.NodeType
const Assertion = node.Assertion

class Context {
  property tree
  property current
  property depth
  property visitor

  constructor(@visitor) {
    @tree = @current = new Node("Charly Unit Testing Framework", NodeType.Root)
    @visitor.on_root(@current)
    @depth = 0
  }

  add_node(type, title, callback) {
    @depth += 1

    const new_node = new Node(title, type)
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

  suite(title, callback) {
    @add_node(NodeType.Suite, title, callback)
  }

  it(title, callback) {
    @add_node(NodeType.Test, title, callback)
  }

  assert(real, expected = null) {
    let assertion

    if arguments.length == 1 {
      assertion = Assertion.single_value(real)
    } else {
      assertion = new Assertion(real, expected)
    }

    @current.push(
      assertion,
      @depth
    )

    @visitor.on_assertion(@current.length() - 1, assertion, @depth)
    self
  }

  catch_exception(e) {
    @assert(e, "No exception to be thrown")
  }
}

export = Context
