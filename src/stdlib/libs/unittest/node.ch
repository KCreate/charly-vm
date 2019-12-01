const NodeType = {
  Root:       0,
  Suite:      1,
  Test:       2,
  Assertion:  3
}

class Node {

  // Contains the next id that will be given to a new node
  static property next_id = 0
  static func get_next_id {
    const id = @next_id + 1
    @next_id += 1
    id
  }

  property id
  property title
  property children
  property type
  property index

  func constructor(@title, @type) {
    @id = Node.get_next_id()
    @children = []
    @index = 0
  }

  func push(node, depth) {
    node.index = @children.length
    @children.push(node)
    self
  }

  func passed() {
    let passed = true
    @children.each(->(child) {
      if passed {
        passed = child.passed()
      }
    })
    passed
  }

  func length() {
    @children.length
  }

  func deep_failed(callback) {
    unless @passed() {
      let path = arguments[1] || []

      if @children.length == 0 {
        callback(path + self)
      } else {
        @children.each(->(child) {
          child.deep_failed(callback, path + self)
        })
      }
    }
  }
}

class Assertion extends Node {
  property expected
  property real
  property has_passed

  func constructor(@real, @expected) {
    @id = Node.get_next_id()
    @title = ""
    @children = []
    @type = NodeType.Assertion
    @has_passed = expected == real
  }

  func passed() {
    @has_passed
  }
}

export.Node = Node
export.NodeType = NodeType
export.Assertion = Assertion
