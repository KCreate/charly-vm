export = {
  num: 25,

  foo: ->{
    "I am foo"
  },

  bar: ->(a, b) {
    a + b
  }
}

export.Person = class Person {
  property name
  property birthyear

  func constructor(@name, @birthyear) = null

  func greeting() {
    @name + " was born in " + @birthyear + "."
  }

}
