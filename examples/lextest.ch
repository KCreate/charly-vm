let i = 0
let lim = 100

func increase {
  i += 1
}

while i < lim {
  print(i)
  increase()
}
