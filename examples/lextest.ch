const print = Charly.internals.get_method

try {
  try {
    throw 25
  } finally {
    print("inside the finally block")
  }
} catch(e) {
  print(e)
}
