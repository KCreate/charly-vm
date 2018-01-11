let foo = func add (a, b) = a + b

try {
  const result = foo(1, 2)
  print(result)
} catch(e) {
  print(e.message)
  print(e.stacktrace)
}
