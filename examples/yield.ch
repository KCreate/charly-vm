func foo {
  return ->{
    yield 25

    try {
      throw "hello world"
    } catch(e) {
      print("caught error inside foo")
      throw "hello world"
    }
  }
}

func bar {
  try {
    return foo()()
  } catch(e) {
    print("caught error inside bar")
  }
}

try {
  const gen = bar()
  print(gen())
  gen()
} catch(e) {
  print("caught error in toplevel")
}
