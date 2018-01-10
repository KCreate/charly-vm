const write = Charly.internals.get_method
const print = ->(arg) {
  write(arg)
  write("\n")
}

export.foo = "this is the export obj"

const might_throw = ->throw "this is an exception"

class A {
  property obj_of_A

  func constructor {
    might_throw()
  }
}

try {
  A()
} catch(e) {
  print(e)
}
