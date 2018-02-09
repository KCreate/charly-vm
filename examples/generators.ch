func create_generator {
  let i = 0

  loop {
    yield i
    i += 1
  }
}

const counter = create_generator()
print(counter()) // => 0
print(counter()) // => 1
print(counter()) // => 2
print(counter()) // => 3
