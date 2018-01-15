const obj = { name: "leonard" }

obj.tap(->(arg) {
  print(arg)
})

Number.prototype.foo = ->{
  print("calling foo")
}

10.times(->(i) {
  print(i)
})

255.foo()
