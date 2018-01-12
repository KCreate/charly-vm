let data = "initial value"
export.read = ->{ return data }
export.write = ->(value) { data = value }

export.someclass = class {
  property name
  property age

  func constructor(name, age) {
    @name = name
    @age = age
  }

  func method {
    print("hello world")
  }
}
