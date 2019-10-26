class Person {
  property name
  property age

  func constructor(@name, @age) = null

  func to_s {
    @name + ":" + @age
  }
}

const L = Person("leonard", 19)
print("a" + L)
