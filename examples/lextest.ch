func gen {
  try {
    yield 1
    throw 2
  } catch(e) {
    print("caught inside gen")
  }

  yield 2
  throw 2
}

const g = gen();

try {
  g()
  g()
} catch(e) {
  print("caught in 2")
}

try {
  g()
} catch(e) {
  print("caught in 3")
}
