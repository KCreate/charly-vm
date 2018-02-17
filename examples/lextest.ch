let template = "hello world my name is leonard"
let str = template

100000.times(->{
  str = template

  9.times(->{
    str *= 2
  })
})
