func create_counter {
  let i = 0

  return func {
    const tmp = i
    i += 1
    return tmp
  }
}

const counter = create_counter()
print(counter()) // => 0
print(counter()) // => 1
print(counter()) // => 2
print(counter()) // => 3
print(counter()) // => 4
