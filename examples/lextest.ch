class Person {
  property name
  property age

  static property registered_persons
  static property max_age

  func constructor(name, age) {
    @name = name
    @age = age

    Person.registered_persons += 1

    if (age > Person.max_age) {
      Person.max_age = age
    }
  }

  func getInfo {
    return [name, age]
  }

  static func getInfo {
    return [registered_persons, max_age]
  }
}
