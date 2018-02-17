func foo {
  print(arguments)

  return func bar {
    print("hello world")

    return func baz {
      print(arguments)
    }
  }
}

const r1 = foo(1, 2, 3)
const r2 = r1(4, 5, 6)
const r3 = r2(7, 8, 9)
