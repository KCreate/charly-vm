












class Foo extends BaseClass {
  property x = []
  property y = {}
  property z = 42

  static property a = 1
  static property b = 2

  constructor(x, y) {

  }

  func foo {}
  bar {}

  // syntax sugar for static property foo = func foo {}
  static func foo {}
  static bar {}
}
