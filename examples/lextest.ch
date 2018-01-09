const print = Charly.internals.get_method

class Test {
  property foo
  property bar

  static property foo
  static property bar

  func constructor() {}
  func member() {}

  static func staticfunc() {}
}

print(Test)
