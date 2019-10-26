let i = defer.interval(100, ->{
  print("hello!")
})

print(i)

defer(->{
  defer.clear_interval(i);
}, 2000)

