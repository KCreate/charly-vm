60.times(func(a) {
  180.times(func(b) {
    let x = 0
    let y = 0
    let i = 0

    while !(x ** 2 + y ** 2 > 4 || i == 99) {
      x = x ** 2 - y ** 2 + b / 60 - 1.5
      y = 2 * x * y + a / 30 - 1
      i += 1
    }

    if i == 99 {
      write("#")
    } else if i <= 10 {
      write(" ")
    } else {
      write(".")
    }
  })

  write("\n")
})
