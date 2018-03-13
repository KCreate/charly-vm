let gen

func counter {
  let i = 0

  loop {
    if i == 2 yield gen()
    yield i
    i += 1
  }
}

try {
  gen = counter()
  print(gen())
  print(gen())
  print(gen())
} catch(e) {
  print(e.message)
}
