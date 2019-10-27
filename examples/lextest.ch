const names = [
  "pipolo",
  "papilu",
  "popila",
  "pipalu"
]

const ivs = names.map(->(name, index) {
  defer.interval(->{
    print(name)
  }, 1000)
})

defer(->{
  ivs.each(->(iv) {
    defer.clear_interval(iv)
  })
}, 1000 * 5)
