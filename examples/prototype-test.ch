class A {
  property name

  func constructor(name) {
    print("A constructor")
    @name = name
  }
}

class B extends A {
  property age

  func constructor(name, age) {
    print("B constructor")
    @age = age
  }
}

class C extends B {
  property height

  func constructor(name, age, height) {
    print("C constructor")
    @height = height
  }
}

const obj = C("leonard", 17, 186)
print(obj)
