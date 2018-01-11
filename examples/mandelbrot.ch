let a = 0
let b = 0

while (a < 60) {
  while (b < 180) {
    let x = 0
    let y = 0
    let i = 0

    while !(x ** 2 + y ** 2 > 4 || i == 99) {
      x = x ** 2 - y ** 2 + b / 60 - 1.5
      y = 2 * x * y + a / 30 - 1
      i += 1
    }

    if i == 99 {
    } else if i <= 10 {
    } else {
    }

    b += 1
  }

  b = 0
  a += 1
}
