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

  greeting() {
    @name + " was born in " + @birthyear + "."
  }

}
