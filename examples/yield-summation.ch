const summer = ->{
  let sum = 0
  sum = yield ->sum

  loop {
    sum += yield
  }
}

const read_state = summer()

summer(100)
summer(100)
summer(100)
summer(100)

print(read_state())
