func throw_some_message(msg) {
  throw msg
}

try {
  throw_some_message("hello world")
} catch(e) {
  print(e) // => hello world
}

try {
  try {
    throw_some_message("this is an error")
  } finally {
    print("this is printed every time")
  }
} catch(e) {
  print(e) // => this is an error
}
