func count_solutions(a, b, c) {
  const tmp = (b ** 2) - (4 * a * c)

  if tmp < 0 return 0
  if tmp == 0 return 1
  if tmp > 0 return 2
}

try {
  loop {
    const a = Charly.io.getn("a: ")
    const b = Charly.io.getn("b: ")
    const c = Charly.io.getn("c: ")

    print(a, b, c)

    const solutions = count_solutions(a, b, c)
    write("There are ", solutions, " solutions!", "\n")
  }
} catch(e) {
  print(e)
}
