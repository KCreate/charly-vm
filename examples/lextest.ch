const write = Charly.internals.get_method
const print = ->(val) {
  write(val)
  write("\n")
}

let a = 0
let b = 0

const s1 = "#"
const s2 = " "
const s3 = "."
const s4 = "\n"

while (a < 60) {
  while (b < 180) {
    let x = 0
    let y = 0
    let i = 0

    while !(x ** 2 + y ** 2 > 4 || i >= 100) {
      x = x ** 2 - y ** 2 + b / 60 - 1.5
      y = 2 * x * y + a / 30 - 1
      i += 1
    }

    if i == 100 {
      write(s1)
    } else if i <= 10 {
      write(s2)
    } else {
      write(s3)
    }

    b += 1
  }

  write(s4)
  b = 0;
  a += 1
}
