class A {
  property foo
  property asdasd
}

class A2 {
  property something_in_a2
}

class B extends A, A2 {
  property bar
}

const obj = B()
print(obj)
