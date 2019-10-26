let i = defer.interval(50, ->{
  print("test")
})

defer(->{
  defer.clear_interval(i);
}, 1000)
