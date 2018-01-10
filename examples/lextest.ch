class Person {
  property name
  property age

  func constructor(name, age) {
    @name = name
    @age = age
  }

  static func new(name, age) {
    self(name, age)
  }
}

const leonard = Person("leonard", 17)
const peter = Person.new("peter", 100)

print(leonard)
print(peter)

if (leonard.name == "leonard") {
  print("leonard.name is set to leonard")
} else {
  print("leonard.name is not set to leonard")
}
