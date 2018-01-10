const print = Charly.internals.get_method

class Person {
  property name
  property age
  property boye

  func constructor(name, age) {
    @name = name
    @age = age
  }
}

const leonard = Person("leonard", 17)

print(leonard)
