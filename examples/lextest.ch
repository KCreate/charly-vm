const print = Charly.internals.get_method

class Test {

  // Duplicate declarations
  property foo
  property foo
  static property bar
  static property bar
  func baz {}
  func baz {}
  static func qux {}
  static func qux {}

  // Shadowing other declaration types
  func foo {}
  property baz
  static func bar {}
  static property qux
}

print(Test)
