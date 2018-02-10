func foo(a) {
  let cond = true

  while cond {
    let a = 25

    yield ->{
      return a
    }

    cond = false
  }

  if cond {
    let a = 30
  }
}
