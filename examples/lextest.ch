func f {
  throw 25
}

func h {
  loop {
    try {
      f()
    } finally {
      return 25
    }
  }

  1 + 1
}

h()

