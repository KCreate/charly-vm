func counter {
  let i = 0
  return ->{
    i += 1
  }
}

const c = counter()

print(c())
print(c())
print(c())
print(c())
print(c())
print(c())
