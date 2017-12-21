let bar

func foo(a, b) {
  let temp = a + b
  temp += bar()
  return temp * 2
}

let result = foo()
