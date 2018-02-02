func foo(a) {
  yield a
}

const gen = foo("argument to generator")
print(gen("arg gen1"))
print(gen("arg gen2"))
print(gen("arg gen3"))
print(gen("arg gen4"))
