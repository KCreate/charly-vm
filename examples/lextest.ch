class Base {
  func method {
    print("Base::method")
  }
}

class Child extends Base {
  func childmethod {
    print("Child::childmethod")
  }
}

const mychild = Child()
mychild.method()
mychild.childmethod()
